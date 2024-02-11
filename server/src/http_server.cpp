#include "http_server.h"

#include "image.h"
#include "uv.h"

#include <sentinel/proto.h>

#include <llhttp.h>

#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include <cstring>

namespace {

struct request final
{
  std::string url;

  void reset() { url.clear(); }
};

struct resource final
{
  std::string content_type;

  std::vector<std::uint8_t> data;
};

using resource_map = std::map<std::string, resource>;

class http_client final
{
public:
  using buffer = std::vector<std::uint8_t>;

  using close_callback = void (*)(void* cb_data, http_client*);

  explicit http_client(uv_loop_t* loop, const resource_map* resources)
    : m_resources(resources)
    , m_telemetry_queue(2)
  {
    uv_tcp_init(loop, &m_socket);

    uv_handle_set_data(reinterpret_cast<uv_handle_t*>(&m_socket), this);

    m_settings.on_url = on_url;

    m_settings.on_message_complete = on_message_complete;

    llhttp_init(&m_parser, HTTP_REQUEST, &m_settings);

    m_parser.data = this;
  }

  void accept(uv_stream_t* server)
  {
    if (uv_accept(server, reinterpret_cast<uv_stream_t*>(&m_socket)) != 0) {
      spdlog::error("Failed to accept client HTTP connection.");
    }
  }

  void close()
  {
    if (!uv_is_closing(to_handle(&m_socket))) {
      uv_close(to_handle(&m_socket), on_close);
    }
  }

  void register_close_callback(void* data, close_callback cb)
  {
    m_close_data = data;
    m_close_cb = cb;
  }

  void start_reading()
  {
    if (uv_read_start(reinterpret_cast<uv_stream_t*>(&m_socket), on_alloc, on_read) != 0) {
      spdlog::error("Failed to start reading from HTTP client.");
      return;
    }
  }

  void publish_telemetry(std::shared_ptr<sentinel::proto::outbound_message>& msg) { m_telemetry_queue.add(msg); }

protected:
  static auto get_self(uv_handle_t* handle) -> http_client*
  {
    return static_cast<http_client*>(uv_handle_get_data(handle));
  }

  static auto get_self(llhttp_t* parser) -> http_client* { return static_cast<http_client*>(parser->data); }

  static void on_alloc(uv_handle_t* handle, size_t size, uv_buf_t* buf)
  {
    auto* self = get_self(handle);
    self->m_read_buffer.resize(size);
    buf->base = reinterpret_cast<char*>(&self->m_read_buffer[0]);
    buf->len = size;
  }

  static void on_read(uv_stream_t* server, const ssize_t read_size, const uv_buf_t* buf)
  {
    auto* self = get_self(to_handle(server));

    if (read_size < 0) {
      self->close();
      return;
    }

    if (llhttp_execute(&self->m_parser, buf->base, static_cast<std::size_t>(read_size)) != HPE_OK) {
      self->close();
    }
  }

  static void on_close(uv_handle_t* handle)
  {
    auto* c = get_self(handle);
    if (c->m_close_cb) {
      c->m_close_cb(c->m_close_data, c);
    }
  }

  static auto on_url(llhttp_t* parser, const char* data, const size_t size) -> int
  {
    auto* self = get_self(parser);

    append_string(data, size, self->m_request.url);

    return HPE_OK;
  }

  static auto on_message_complete(llhttp_t* parser) -> int
  {
    get_self(parser)->handle_request();
    return HPE_OK;
  }

  static void append_string(const char* data, size_t size, std::string& out)
  {
    const auto prev_size = out.size();

    out.resize(prev_size + size);

    std::memcpy(&out[prev_size], data, size);
  }

  void handle_request()
  {
    switch (m_parser.method) {
      case HTTP_GET:
        handle_get_request();
        break;
      default:
        respond(500);
        break;
    }

    m_request.reset();
  }

  void handle_get_request()
  {
    std::string url = m_request.url;

    if (url == "/") {
      url = "/index.html";
    }

    {
      auto it = m_resources->find(url);

      if (it != m_resources->end()) {
        respond(200, it->second.content_type.c_str(), it->second.data);
        return;
      }
    }

    if (url == "/api/stream") {
      respond(200, "application/octet-stream", get_latest_update());
      return;
    }

    respond(404);
  }

  void respond(const int status, const char* type = nullptr, const std::vector<std::uint8_t>& content = {})
  {
    std::ostringstream header_stream;
    header_stream << "HTTP/1.1 " << status << "\r\n";
    if (type != nullptr) {
      header_stream << "Content-Type: " << type << "\r\n";
    }
    header_stream << "Content-Length: " << content.size() << "\r\n";
    header_stream << "\r\n";
    const auto header = header_stream.str();

    std::vector<std::uint8_t> out;
    out.resize(header.size() + content.size());
    std::memcpy(&out[0], header.data(), header.size());
    std::memcpy(&out[header.size()], content.data(), content.size());

    write_operation::send(reinterpret_cast<uv_stream_t*>(&m_socket), std::move(out), nullptr, nullptr);
  }

  auto get_latest_update() -> std::vector<std::uint8_t>
  {
    if (m_telemetry_queue.empty()) {

      static const std::vector<std::uint8_t> empty_response;

      return empty_response;
    }

    auto buf = m_telemetry_queue.aggregate()->buffer;

    m_telemetry_queue.clear();

    return std::move(*buf);
  }

private:
  close_callback m_close_cb{ nullptr };

  void* m_close_data{ nullptr };

  uv_tcp_t m_socket{};

  std::vector<std::uint8_t> m_read_buffer;

  llhttp_settings_t m_settings{};

  llhttp_t m_parser{};

  request m_request;

  sentinel::proto::queue m_telemetry_queue;

  const resource_map* m_resources{ nullptr };
};

class http_server_impl final : public http_server
{
public:
  http_server_impl(uv_loop_t* loop)
  {
    uv_tcp_init(loop, &m_server);

    uv_handle_set_data(to_handle(&m_server), this);
  }

  void close() override
  {
    uv_close(to_handle(&m_server), nullptr);

    for (auto& c : m_clients) {
      c->close();
    }
  }

  auto setup(const char* ip, int port) -> bool override
  {
    sockaddr_in address{};

    if (uv_ip4_addr(ip, port, &address) != 0) {
      spdlog::error("Failed to parse IP address '{}:{}'.", ip, port);
      return false;
    }

    if (uv_tcp_bind(&m_server, reinterpret_cast<const sockaddr*>(&address), 0) != 0) {
      spdlog::error("Failed to bind HTTP server to '{}:{}'.", ip, port);
      return false;
    }

    if (uv_listen(reinterpret_cast<uv_stream_t*>(&m_server), 128, on_connect) != 0) {
      spdlog::error("Failed to start listening for incoming HTTP connections.");
      return false;
    }

    spdlog::info("Listening for HTTP connections on '{}:{}'.", ip, port);

    return true;
  }

  void add_file(std::string path, std::string content_type, std::vector<std::uint8_t> data) override
  {
    m_resources.emplace(std::move(path), resource{ std::move(content_type), std::move(data) });
  }

  void publish_telemetry(std::shared_ptr<sentinel::proto::outbound_message>& msg) override
  {
    for (auto& c : m_clients) {
      c->publish_telemetry(msg);
    }
  }

protected:
  static auto get_self(uv_handle_t* handle) -> http_server_impl*
  {
    return static_cast<http_server_impl*>(uv_handle_get_data(handle));
  }

  static void on_client_close(void* self_ptr, http_client* c)
  {
    auto* self = static_cast<http_server_impl*>(self_ptr);

    for (auto it = self->m_clients.begin(); it != self->m_clients.end(); it++) {
      if (it->get() == c) {
        self->m_clients.erase(it);
        return;
      }
    }

    spdlog::info("Closed HTTP client connection.");
  }

  static void on_connect(uv_stream_t* server, int status)
  {
    if (status != 0) {
      spdlog::error("Failed to accept client HTTP connection.");
      return;
    }

    spdlog::info("Connected to new HTTP client.");

    auto* self = get_self(to_handle(server));

    auto* loop = uv_handle_get_loop(to_handle(server));

    auto c = std::make_unique<http_client>(loop, &self->m_resources);

    c->accept(server);

    c->register_close_callback(self, on_client_close);

    c->start_reading();

    self->m_clients.emplace_back(std::move(c));
  }

private:
  uv_tcp_t m_server{};

  std::vector<std::unique_ptr<http_client>> m_clients;

  std::shared_ptr<sentinel::proto::outbound_message> m_latest_update;

  float m_anomaly_level{ 1 };

  resource_map m_resources;
};

} // namespace

auto
http_server::create(uv_loop_t* loop) -> std::unique_ptr<http_server>
{
  return std::make_unique<http_server_impl>(loop);
}

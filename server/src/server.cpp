#include "server.h"

#include "image.h"

#include <motion_monitor_proto.h>

#include <uv.h>

#include <algorithm>
#include <vector>

#include <spdlog/spdlog.h>

#include <cstring>

namespace {

auto
to_handle(uv_tcp_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

auto
to_handle(uv_stream_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

auto
to_handle(uv_write_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

class write_operation final
{
public:
  using complete_cb = void (*)(void*, bool success);

  static void send(uv_stream_t* socket, std::vector<std::uint8_t> data, void* cb_data, complete_cb cb_func)
  {
    auto* op = new write_operation(std::move(data), cb_data, cb_func);

    if (!op->send(socket)) {
      spdlog::error("Failed to send message.");
      if (cb_func) {
        cb_func(cb_data, false);
      }
      delete op;
    }
  }

protected:
  write_operation(std::vector<std::uint8_t> data, void* cb_data, complete_cb cb_func)
    : m_data(std::move(data))
    , m_cb_data(cb_data)
    , m_cb_func(cb_func)
  {
    m_buffer.base = reinterpret_cast<char*>(&m_data[0]);
    m_buffer.len = m_data.size();

    uv_handle_set_data(to_handle(&m_handle), this);
  }

  auto send(uv_stream_t* socket) -> bool { return uv_write(&m_handle, socket, &m_buffer, 1, on_write_complete) == 0; }

  static void on_write_complete(uv_write_t* handle, int status)
  {
    auto* self = static_cast<write_operation*>(uv_handle_get_data(to_handle(handle)));

    if (self->m_cb_func) {
      self->m_cb_func(self->m_cb_data, status == 0);
    }

    delete self;

    if (status != 0) {
      spdlog::error("Failed to complete write operation.");
    }
  }

private:
  std::vector<std::uint8_t> m_data;

  uv_buf_t m_buffer{};

  uv_write_t m_handle{};

  void* m_cb_data{ nullptr };

  complete_cb m_cb_func{ nullptr };
};

template<typename Scalar>
auto
clamp(Scalar x, Scalar min, Scalar max) -> Scalar
{
  return std::max(std::min(x, max), min);
}

auto
encode_message(const image& img, const float motion)
{
  const auto motion_u16 = clamp(static_cast<int>(motion * 65535), 0, 65535);

  const std::uint16_t header[4]{ static_cast<std::uint16_t>(img.width),
                                 static_cast<std::uint16_t>(img.height),
                                 static_cast<std::uint16_t>(img.channels),
                                 static_cast<std::uint16_t>(motion_u16) };

  motion_monitor::writer writer("image::update", img.data.size() + sizeof(header));

  writer.write(header, sizeof(header));

  writer.write(img.data.data(), img.data.size());

  return writer.complete();
}

class client final
{
public:
  using close_cb = void (*)(void* data, client* c);

  client(uv_loop_t* loop)
  {
    uv_handle_set_data(to_handle(&m_socket), this);

    uv_tcp_init(loop, &m_socket);
  }

  void set_close_callback(void* data, close_cb cb)
  {
    m_close_data = data;
    m_close_cb = cb;
  }

  void close()
  {
    if (!uv_is_closing(to_handle(&m_socket))) {
      uv_close(to_handle(&m_socket), on_close);
    }
  }

  auto accept(uv_stream_t* server) -> bool
  {
    if (uv_accept(server, reinterpret_cast<uv_stream_t*>(&m_socket)) != 0) {
      return false;
    }

    return true;
  }

  auto start_reading() -> bool
  {
    return uv_read_start(reinterpret_cast<uv_stream_t*>(&m_socket), on_alloc, on_read) == 0;
  }

  void publish_frame(const image& img, const float motion)
  {
    if (!m_ready) {
      return;
    }

    auto data = encode_message(img, motion);

    write_operation::send(reinterpret_cast<uv_stream_t*>(&m_socket), std::move(data), this, on_image_write_complete);

    m_ready = false;
  }

protected:
  static auto get_self(uv_handle_t* handle) -> client* { return static_cast<client*>(uv_handle_get_data(handle)); }

  static void on_image_write_complete(void* self_ptr, const bool success)
  {
    if (!success) {
      static_cast<client*>(self_ptr)->m_ready = true;
    }
  }

  static void on_close(uv_handle_t* handle)
  {
    auto* self = get_self(handle);

    if (self->m_close_cb) {
      self->m_close_cb(self->m_close_data, self);
    }
  }

  static void on_alloc(uv_handle_t* handle, size_t size, uv_buf_t* buf)
  {
    auto* self = get_self(handle);

    self->m_read_buffer.resize(self->m_read_size + size);

    buf->base = self->m_read_buffer.data() + self->m_read_size;

    buf->len = size;
  }

  static void on_read(uv_stream_t* stream, ssize_t read_size, const uv_buf_t* /* buf */)
  {
    auto* self = get_self(to_handle(stream));

    if (read_size < 0) {
      self->close();
      return;
    }

    self->m_read_size += static_cast<std::size_t>(read_size);

    self->attempt_unpack_message();
  }

  void attempt_unpack_message()
  {
    const auto result = motion_monitor::read(reinterpret_cast<const std::uint8_t*>(m_read_buffer.data()), m_read_size);

    if (result.payload_ready) {
      handle_message(result);
    }

    m_read_size -= result.cull_size;

    m_read_buffer.erase(m_read_buffer.begin(), m_read_buffer.begin() + result.cull_size);
  }

  void handle_message(const motion_monitor::read_result& res)
  {
    if (res.type_id == "ready") {
      m_ready = true;
    }
  }

private:
  uv_tcp_t m_socket{};

  void* m_close_data{ nullptr };

  close_cb m_close_cb{ nullptr };

  std::vector<char> m_read_buffer;

  std::size_t m_read_size{};

  bool m_ready{ true };
};

class server_impl final : public server
{
public:
  explicit server_impl(uv_loop_t* loop)
  {
    uv_handle_set_data(to_handle(&m_socket), this);

    uv_tcp_init(loop, &m_socket);
  }

  ~server_impl() override {}

  auto setup(const char* ip, int port) -> bool override
  {
    sockaddr_in address{};

    int r = uv_ip4_addr(ip, port, &address);
    if (r != 0) {
      spdlog::error("Failed to parse server bind address.");
      return false;
    }

    r = uv_tcp_bind(&m_socket, reinterpret_cast<const sockaddr*>(&address), 0);
    if (r != 0) {
      spdlog::error("Failed to bind server socket to {}:{}.", ip, port);
      return false;
    }

    r = uv_listen(reinterpret_cast<uv_stream_t*>(&m_socket), 128, on_connect);
    if (r != 0) {
      spdlog::error("Failed to start listening for incoming connections.");
      return false;
    }

    spdlog::info("Server is listening for incoming connections.");

    return true;
  }

  void close() override
  {
    if (!uv_is_closing(to_handle(&m_socket))) {

      uv_close(to_handle(&m_socket), nullptr);
    }

    for (auto& c : m_clients) {
      c->close();
    }
  }

  void publish_frame(const image& img, const float motion) override
  {
    for (auto& c : m_clients) {
      c->publish_frame(img, motion);
    }
  }

protected:
  static auto get_self(uv_handle_t* handle) -> server_impl*
  {
    return static_cast<server_impl*>(uv_handle_get_data(handle));
  }

  static void on_connect(uv_stream_t* server, int status)
  {
    if (status != 0) {
      spdlog::error("Failed to listen for incoming connection.");
      return;
    }

    auto* self = get_self(to_handle(server));

    auto c = std::make_unique<client>(uv_handle_get_loop(to_handle(&self->m_socket)));

    c->set_close_callback(self, on_client_close);

    if (c->accept(reinterpret_cast<uv_stream_t*>(&self->m_socket))) {
      if (!c->start_reading()) {
        spdlog::error("Failed to start reading from client socket.");
      }
    } else {
      spdlog::error("Failed to accept incoming connection.");
    }

    spdlog::info("Connected to client.");

    self->m_clients.emplace_back(std::move(c));
  }

  static void on_client_close(void* self_ptr, client* c)
  {
    auto* self = static_cast<server_impl*>(self_ptr);

    for (auto it = self->m_clients.begin(); it != self->m_clients.end(); it++) {
      if (it->get() == c) {
        spdlog::info("Closing client connection.");
        self->m_clients.erase(it);
        return;
      }
    }
  }

private:
  uv_tcp_t m_socket{};

  std::vector<std::unique_ptr<client>> m_clients;
};

} // namespace

auto
server::create(uv_loop_t* loop) -> std::unique_ptr<server>
{
  return std::make_unique<server_impl>(loop);
}

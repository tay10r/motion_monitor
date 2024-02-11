#include <sentinel/client.h>
#include <sentinel/proto.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <cstdlib>

#include <stb_image_write.h>

namespace {

using namespace sentinel;

class observer_impl final
  : public client::observer
  , public proto::payload_visitor_base
{
public:
  explicit observer_impl(client::connection* conn, const int num_frames)
    : m_connection(conn)
    , m_max_frames(num_frames)
  {
  }

  void on_error(const char* what) override { std::cerr << what << std::endl; }

  void on_connection_established() override { std::cout << "connection established" << std::endl; }

  void on_connection_failed() override { std::cout << "connection failed" << std::endl; }

  void on_connection_closed() override { std::cout << "connection closed" << std::endl; }

  void on_payload(const std::string& type, const void* payload, const std::size_t payload_size) override
  {
    proto::decode_payload(type, payload, payload_size, *this);
  }

  void visit_monochrome_camera_update(const std::uint8_t* data,
                                      const std::uint16_t w,
                                      const std::uint16_t h,
                                      const std::uint64_t time,
                                      const std::uint32_t sensor_id) override
  {
    std::cout << "on monochrome image" << std::endl;
    //
    //
  }

  void visit_rgb_camera_update(const std::uint8_t* data,
                               const std::uint16_t w,
                               const std::uint16_t h,
                               const std::uint64_t time,
                               const std::uint32_t sensor_id) override
  {
    std::cout << "on image" << std::endl;

    std::ostringstream path_stream;
    path_stream << std::setw(8) << std::setfill('0') << m_frame_counter << ".png";
    const auto path = path_stream.str();

    std::cout << "saving frame '" << path << "'" << std::endl;

    stbi_write_png(path.c_str(), w, h, 3, data, w * 3);

    m_frame_counter++;
  }

  void visit_microphone_update(const std::int16_t* data,
                               const std::uint32_t size,
                               const std::uint32_t sample_rate,
                               const std::uint64_t time,
                               const std::uint32_t sensor_id) override
  {
  }

  void visit_temperature_update(const float temperature,
                                const std::uint64_t time,
                                const std::uint32_t sensor_id) override
  {
  }

  auto visit_unknown_payload(const std::string& type, const void* payload, const std::size_t payload_size)
    -> bool override
  {
    std::cerr << "received unknown payload type '" << type << "'." << std::endl;
    return false;
  }

  void next_frame() { m_connection->notify_ready(); }

  auto done() const -> bool { return m_frame_counter >= m_max_frames; }

private:
  client::connection* m_connection{ nullptr };

  int m_frame_counter{ 0 };

  int m_max_frames{ 1 };
};

void
on_timer_expire(uv_timer_t* timer)
{
  auto* observer = static_cast<observer_impl*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(timer)));

  if (observer->done()) {
    uv_stop(uv_handle_get_loop(reinterpret_cast<uv_handle_t*>(timer)));
  } else {
    observer->next_frame();
  }
}

} // namespace

int
main(int argc, char** argv)
{
  const char* ip = "127.0.0.1";

  int interval{ 60'000 };

  int num_frames{ 1 };

  if (argc > 1) {
    ip = argv[1];
  }

  if (argc > 2) {
    interval = std::atoi(argv[2]);
  }

  if (argc > 3) {
    num_frames = std::atoi(argv[3]);
  }

  uv_loop_t loop{};

  uv_loop_init(&loop);

  uv_timer_t timer{};

  uv_timer_init(&loop, &timer);

  uv_timer_start(&timer, on_timer_expire, 0, interval);

  auto connection = client::connection::create(&loop, /* handle interrupt */ true);

  connection->set_streaming_enabled(false);

  observer_impl obs(connection.get(), num_frames);

  uv_handle_set_data(reinterpret_cast<uv_handle_t*>(&timer), &obs);

  connection->add_observer(&obs);

  connection->connect(ip, 5100);

  uv_run(&loop, UV_RUN_DEFAULT);

  std::cout << "shutting down" << std::endl;

  connection->close();

  uv_close(reinterpret_cast<uv_handle_t*>(&timer), nullptr);

  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);

  return EXIT_SUCCESS;
}

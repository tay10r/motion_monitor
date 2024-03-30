#include <sentinel/client.h>
#include <sentinel/proto.h>

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <cstdlib>

#include "tinywav.h"

namespace {

using namespace sentinel;

class observer_impl final
  : public client::observer
  , public proto::payload_visitor
{
public:
  explicit observer_impl(client::connection* conn)
    : m_connection(conn)
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

  void visit_monochrome_camera_frame_event(const proto::camera_frame_event& ev) override
  {
    std::cout << "on monochrome image" << std::endl;
    //
    //
  }

  void visit_rgb_camera_frame_event(const proto::camera_frame_event& ev) override {}

  void visit_microphone_update(const std::int16_t* data,
                               const std::uint32_t size,
                               const std::uint32_t sample_rate,
                               const std::uint64_t time,
                               const std::uint32_t sensor_id) override
  {
    m_sample_rate = sample_rate;

    for (std::uint32_t i = 0; i < size; i++) {

      const float v = static_cast<float>(data[i]) / static_cast<float>(std::numeric_limits<std::int16_t>::max());

      m_samples.emplace_back(v);
    }

    std::cout << "received audio (samples=" << m_samples.size() << ")" << std::endl;
  }

  void visit_temperature_update(const float temperature, const std::uint64_t time, const std::uint32_t) override {}

  auto visit_unknown_payload(const std::string& type, const void* payload, const std::size_t payload_size)
    -> bool override
  {
    std::cerr << "received unknown payload type '" << type << "'." << std::endl;
    return false;
  }

  void next_frame() { m_connection->notify_ready(); }

  void save_wav(const char* filename)
  {
    TinyWav file;

    tinywav_open_write(&file, 1, m_sample_rate, TW_INT16, TW_INLINE, filename);

    tinywav_write_f(&file, &m_samples[0], m_samples.size());

    tinywav_close_write(&file);
  }

private:
  client::connection* m_connection{ nullptr };

  std::vector<float> m_samples;

  std::uint32_t m_sample_rate{};
};

} // namespace

int
main(int argc, char** argv)
{
  const char* ip = "127.0.0.1";

  if (argc > 1) {
    ip = argv[1];
  }

  uv_loop_t loop{};

  uv_loop_init(&loop);

  auto connection = client::connection::create(&loop, /* handle interrupt */ true);

  observer_impl obs(connection.get());

  connection->add_observer(&obs);

  connection->connect(ip, 5100);

  connection->notify_ready();

  uv_run(&loop, UV_RUN_DEFAULT);

  std::cout << "shutting down" << std::endl;

  connection->close();

  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);

  obs.save_wav("audio.wav");

  return EXIT_SUCCESS;
}

#include "fetch_request.h"

#include <sentinel/proto.h>

#include <fstream>
#include <random>
#include <string>

#include <iostream>

namespace {

using namespace sentinel::proto;

class fetch_request_impl final : public fetch_request
{
public:
  explicit fetch_request_impl(const char* path)
    : m_path(path)
  {
  }

  auto done() const -> bool override { return m_done; }

  auto success() const -> bool override { return m_success; }

  auto get_data() const -> const void* override { return m_data.data(); }

  auto get_size() const -> std::size_t override { return m_data.size(); }

  void iterate() override
  {
    if (m_done) {
      return;
    }

    m_done = true;

    if (m_path == "api/stream") {
      get_stream_data();
    } else {
      open_file();
    }

    m_success = true;
  }

protected:
  auto get_microphone_data() -> std::shared_ptr<outbound_message>
  {
    constexpr std::size_t sample_rate{ 44100 };

    constexpr std::uint32_t sensor_id{ 1 };

    std::vector<std::int16_t> samples(1024);

    constexpr float tone{ 440.0f };

    std::uniform_real_distribution<float> noise_dist(-1e-1, 1e-1);

    constexpr float amplitude{ 10'000 };

    const float dt = 1.0f / sample_rate;

    const auto tstamp = static_cast<std::uint64_t>(m_time * 1e6);

    for (std::size_t i = 0; i < samples.size(); i++) {

      const std::int16_t v = static_cast<std::int16_t>((noise_dist(m_rng) + std::sin(tone * m_time)) * amplitude);

      samples[i] = v;

      m_time += dt;
    }

    auto update = writer::create_microphone_update(samples.data(), samples.size(), sample_rate, tstamp, sensor_id);

    return update;
  }

  void get_stream_data()
  {
    auto msg = get_microphone_data();

    const auto* ptr = reinterpret_cast<const char*>(msg->buffer->data());

    const auto len = msg->buffer->size();

    m_data = std::string(ptr, len);
  }

  void open_file()
  {
    std::ifstream file(m_path);

    if (!file.good()) {
      return;
    }

    file.seekg(0, std::ios::end);

    const auto file_size = file.tellg();
    if (file_size == -1l) {
      return;
    }

    file.seekg(0, std::ios::beg);

    m_data.resize(file_size);

    file.read(&m_data[0], m_data.size());
  }

private:
  static double m_time;

  std::string m_path;

  bool m_done{ false };

  bool m_success{ false };

  std::string m_data;

  std::mt19937 m_rng{ 0 };
};

double fetch_request_impl::m_time = 0;

} // namespace

auto
fetch_request::create(const char* path) -> std::unique_ptr<fetch_request>
{
  return std::make_unique<fetch_request_impl>(path);
}

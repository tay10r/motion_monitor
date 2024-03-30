#include "telemetry_stream.h"

#include <motion_monitor_proto.h>

#include <chrono>
#include <random>
#include <vector>

namespace {

class generator
{
public:
  virtual ~generator() = default;

  virtual auto generate() -> std::vector<std::uint8_t> = 0;
};

class camera_generator final : public generator
{
public:
  auto generate() -> std::vector<std::uint8_t> override
  {
    const auto w = 640;
    const auto h = 480;
    std::vector<std::uint8_t> data(w * h * 3, 0);

    std::uniform_int_distribution<int> dist(0, 255);

    for (int i = 0; i < (w * h); i++) {
      data[i * 3 + 0] = dist(m_rng);
      data[i * 3 + 1] = dist(m_rng);
      data[i * 3 + 2] = dist(m_rng);
    }

    m_time += 100'000'000;

    return motion_monitor::writer::create_rgb_camera_update(data.data(), w, h, m_time, 0);
  }

private:
  std::mt19937 m_rng{ 0 };

  std::uint64_t m_time{ 0 };
};

class temperature_generator final : public generator
{
public:
  auto generate() -> std::vector<std::uint8_t> override
  {
    std::uniform_real_distribution<float> dist(70.4, 71.85);

    m_time += 100'000'000;

    return motion_monitor::writer::create_temperature_update(dist(m_rng), m_time, 1);
  }

private:
  std::mt19937 m_rng{ 0 };

  std::uint64_t m_time{ 0 };
};

class telemetry_stream_impl final : public telemetry_stream
{
public:
  using clock_type = std::chrono::high_resolution_clock;

  using time_point_type = typename clock_type::time_point;

  telemetry_stream_impl(int interval_ms = 100)
    : m_start_time(clock_type::now())
    , m_interval_ms(interval_ms)
  {
  }

  void request_new_telemetry() override { m_requested = true; }

  auto check_new_telemetry() -> bool override
  {
    if (!m_requested) {
      return false;
    }

    const auto t = clock_type::now();

    const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(t - m_start_time).count();

    const auto num_updates_required = dt / m_interval_ms;

    auto new_telemetry = false;

    if (num_updates_required > m_num_updates) {

      m_num_updates = num_updates_required;

      const auto buf = m_generator->generate();

      for (auto* o : m_observers) {
        o->observe(buf.data(), buf.size());
      }

      new_telemetry = true;

      m_requested = false;
    }

    return new_telemetry;
  }

  void add_observer(observer* o) override { m_observers.emplace_back(o); }

private:
  std::vector<observer*> m_observers;

  time_point_type m_start_time;

  int m_interval_ms{ 0 };

  int m_num_updates{ 0 };

  std::unique_ptr<generator> m_generator;

  bool m_requested{ false };
};

} // namespace

auto
telemetry_stream::create() -> std::unique_ptr<telemetry_stream>
{
  return std::unique_ptr<telemetry_stream>(new telemetry_stream_impl());
}

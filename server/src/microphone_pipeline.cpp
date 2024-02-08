#include "microphone_pipeline.h"

#include "microphone_device.h"

#include <motion_monitor_proto.h>

namespace {

class microphone_pipeline_impl final : public microphone_pipeline
{
public:
  explicit microphone_pipeline_impl(const config::microphone_config& cfg)
    : m_config(cfg)
  {
  }

  auto loop(bool& should_close) -> std::vector<std::vector<std::uint8_t>> override
  {
    if (!m_device) {
      m_device = microphone_device::create(m_config.name.c_str(), m_config.rate);

      if (!m_device->is_open()) {
        should_close = true;
        return {};
      }

      m_sample_rate = m_device->get_rate();
    }

    const auto samples = m_device->read();

    const auto time = 0;

    auto buf = motion_monitor::writer::create_microphone_update(
      samples.data(), samples.size(), m_sample_rate, time, m_config.sensor_id);

    buffer_vec buffers;

    buffers.emplace_back(std::move(buf));

    return buffers;
  }

private:
  const config::microphone_config m_config;

  std::unique_ptr<microphone_device> m_device;

  std::uint32_t m_sample_rate{};
};

} // namespace

auto
microphone_pipeline::create(const config::microphone_config& cfg) -> std::unique_ptr<microphone_pipeline>
{
  return std::make_unique<microphone_pipeline_impl>(cfg);
}

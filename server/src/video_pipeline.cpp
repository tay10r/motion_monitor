#include "video_pipeline.h"

#include "clock.h"
#include "image.h"
#include "video_device.h"

#include <sentinel/proto.h>

namespace {

class video_pipeline_impl final : public video_pipeline
{
public:
  explicit video_pipeline_impl(const config::camera_config& cfg)
    : m_config(cfg)
  {
  }

  auto loop(bool& should_close) -> std::vector<std::shared_ptr<sentinel::proto::outbound_message>> override
  {
    if (!m_device) {

      m_device = video_device::create();

      m_opened = m_device->open(m_config.device_index);
    }

    const auto img = m_device->read_frame();

    auto msg = sentinel::proto::writer::create_rgb_camera_update(
      img.data.data(), img.width, img.height, get_clock_time(), m_config.sensor_id);

    return { std::move(msg) };
  }

private:
  config::camera_config m_config;

  std::unique_ptr<video_device> m_device;

  bool m_opened{ false };
};

} // namespace

auto
video_pipeline::create(const config::camera_config& cfg) -> std::unique_ptr<video_pipeline>
{
  return std::make_unique<video_pipeline_impl>(cfg);
}

#include "video_pipeline.h"

#include "clock.h"
#include "image.h"
#include "video_device.h"
#include "video_storage.h"

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
    if (m_config.storage_enabled) {
      if (!m_storage) {
        m_storage = video_storage::create(m_config.storage_path,
                                          m_config.storage_quality,
                                          m_config.storage_days,
                                          m_config.storage_width,
                                          m_config.storage_height,
                                          m_config.storage_rate);
      }
    }

    if (!m_device) {

      m_device = video_device::create();

      m_opened = m_device->open(m_config.device_index, m_config.frame_width, m_config.frame_height);
    }

    auto img = m_device->read_frame();

    if (!img.has_value()) {
      return {};
    }

    if (m_storage) {
      m_storage->store(img.value());
    }

    auto msg = sentinel::proto::writer::create_rgb_camera_update(
      img->data.data(), img->width, img->height, get_clock_time(), m_config.sensor_id, {}, m_config.jpeg_quality);

    return { std::move(msg) };
  }

private:
  config::camera_config m_config;

  std::unique_ptr<video_device> m_device;

  std::unique_ptr<video_storage> m_storage;

  bool m_opened{ false };
};

} // namespace

auto
video_pipeline::create(const config::camera_config& cfg) -> std::unique_ptr<video_pipeline>
{
  return std::make_unique<video_pipeline_impl>(cfg);
}

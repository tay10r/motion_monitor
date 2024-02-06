#include "camera_pipeline.h"

#include "detector.h"
#include "image.h"
#include "uv.h"
#include "video_device.h"

namespace {

class camera_pipeline_impl final : public camera_pipeline
{
public:
  camera_pipeline_impl(uv_loop_t* loop, const config::camera_config& cfg)
    : m_sensor_id(cfg.sensor_id)
  {
    if (!cfg.detector_path.empty()) {
      m_detector = detector::create(cfg.detector_path.c_str());
    }

    uv_timer_init(loop, &m_timer);

    uv_handle_set_data(to_handle(&m_timer), this);

    m_video_device = video_device::create();

    m_is_open = m_video_device->open(cfg.device_index);

    if (m_is_open) {
      uv_timer_start(&m_timer, on_timer_expire, 0, cfg.read_interval);
    }
  }

  void close() override { uv_close(to_handle(&m_timer), nullptr); }

  void add_observer(observer* o) override { m_observers.emplace_back(o); }

protected:
  static void on_timer_expire(uv_timer_t* timer)
  {
    auto* self = static_cast<camera_pipeline_impl*>(uv_handle_get_data(to_handle(timer)));

    const auto img = self->m_video_device->read_frame();

    float anomaly_level{ 1 };

    if (self->m_detector) {
      anomaly_level = self->m_detector->exec(img);
    }

    for (auto* o : self->m_observers) {
      o->on_image_update(img, self->m_sensor_id, anomaly_level);
    }
  }

private:
  uv_timer_t m_timer{};

  std::unique_ptr<video_device> m_video_device;

  bool m_is_open{ false };

  std::vector<observer*> m_observers;

  std::unique_ptr<detector> m_detector;

  std::uint32_t m_sensor_id{};
};

} // namespace

auto
camera_pipeline::create(uv_loop_t* loop, const config::camera_config& cfg) -> std::unique_ptr<camera_pipeline>
{
  return std::make_unique<camera_pipeline_impl>(loop, cfg);
}

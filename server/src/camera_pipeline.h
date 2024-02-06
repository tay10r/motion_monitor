#pragma once

#include <uv.h>

#include <memory>

#include "config.h"
#include "image.h"

class camera_pipeline
{
public:
  class observer
  {
  public:
    virtual ~observer() = default;

    virtual void on_image_update(const image& img, const std::uint32_t sensor_id, const float anomaly_level) = 0;
  };

  static auto create(uv_loop_t* loop, const config::camera_config& cfg) -> std::unique_ptr<camera_pipeline>;

  virtual ~camera_pipeline() = default;

  virtual void close() = 0;

  virtual void add_observer(observer* o) = 0;
};

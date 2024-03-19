#pragma once

#include "config.h"

#include <memory>

struct image;
class video_device;

class exposure
{
public:
  static auto create(config::camera_exposure_mode mode) -> std::unique_ptr<exposure>;

  virtual ~exposure() = default;

  virtual void update(video_device& dev, const image& img) = 0;
};

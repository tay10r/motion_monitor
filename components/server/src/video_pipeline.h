#pragma once

#include <memory>

#include "config.h"
#include "pipeline.h"

class video_pipeline : public pipeline
{
public:
  static auto create(const config::camera_config& cfg) -> std::unique_ptr<video_pipeline>;

  virtual ~video_pipeline() = default;
};

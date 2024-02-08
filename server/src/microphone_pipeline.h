#pragma once

#include <memory>

#include "config.h"
#include "pipeline.h"

class microphone_pipeline : public pipeline
{
public:
  static auto create(const config::microphone_config& cfg) -> std::unique_ptr<microphone_pipeline>;

  virtual ~microphone_pipeline() = default;
};

#pragma once

#include "config.h"

#include <chrono>
#include <memory>

struct image;

class video_storage
{
public:
  using clock_type = std::chrono::system_clock;

  using time_point = typename clock_type::time_point;

  static auto create(std::string path, float quality, float days, int storage_width, int storage_height)
    -> std::unique_ptr<video_storage>;

  virtual ~video_storage() = default;

  virtual void store(const image& img) = 0;
};

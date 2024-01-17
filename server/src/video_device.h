#pragma once

#include <memory>

struct image;

class video_device
{
public:
  static auto create() -> std::unique_ptr<video_device>;

  virtual ~video_device() = default;

  virtual auto open(int device_index) -> bool = 0;

  virtual auto read_frame() -> image = 0;
};

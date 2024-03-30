#pragma once

#include <memory>
#include <optional>

struct image;

class video_device
{
public:
  static auto create() -> std::unique_ptr<video_device>;

  virtual ~video_device() = default;

  virtual auto open(int device_index, int frame_w, int frame_h) -> bool = 0;

  virtual auto read_frame() -> std::optional<image> = 0;

  virtual void set_manual_exposure_enabled(bool enabled) = 0;

  virtual void set_exposure(float exposure) = 0;

  virtual auto get_exposure() const -> float = 0;
};

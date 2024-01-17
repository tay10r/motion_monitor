#pragma once

#include <memory>

struct image;

class motion_filter
{
public:
  enum class result
  {
    keep,
    discard
  };

  static auto create() -> std::unique_ptr<motion_filter>;

  virtual ~motion_filter() = default;

  /**
   *
   * @brief A value from 0 to 1 that indicates how much motion is in the image. Zero means there is no motion.
   * */
  virtual auto filter(const image& img) -> float = 0;
};

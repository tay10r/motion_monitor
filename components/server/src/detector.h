#pragma once

#include <memory>

struct image;

class detector
{
public:
  static auto create(const char* path) -> std::unique_ptr<detector>;

  virtual ~detector() = default;

  /**
   *
   * @brief A value from 0 to 1 that indicates how much motion is in the image. Zero means there is no motion.
   * */
  virtual auto exec(const image& img) -> float = 0;
};

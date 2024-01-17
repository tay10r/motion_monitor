#pragma once

#include <memory>

class algo
{
public:
  static auto create(const char* config) -> std::unique_ptr<algo>;

  virtual ~algo() = default;
};

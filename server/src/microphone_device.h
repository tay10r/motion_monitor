#pragma once

#include <memory>
#include <vector>

#include <cstdint>

class microphone_device
{
public:
  static auto create(const char* device, unsigned int sampling_rate) -> std::unique_ptr<microphone_device>;

  virtual ~microphone_device() = default;

  virtual auto is_open() const -> bool = 0;

  virtual auto get_rate() const -> std::uint32_t = 0;

  virtual auto read() -> std::vector<std::int16_t> = 0;
};

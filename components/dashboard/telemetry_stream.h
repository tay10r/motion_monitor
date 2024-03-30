#pragma once

#include <memory>
#include <string>

#include <cstddef>
#include <cstdint>

#include "config.h"

class telemetry_stream
{
public:
  class observer
  {
  public:
    virtual ~observer() = default;

    virtual void observe(const void* message_data, const std::size_t message_size) = 0;
  };

  static auto create() -> std::unique_ptr<telemetry_stream>;

  virtual ~telemetry_stream() = default;

  virtual void request_new_telemetry() = 0;

  virtual auto check_new_telemetry() -> bool = 0;

  virtual void add_observer(observer* o) = 0;
};

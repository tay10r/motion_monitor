#pragma once

#include <memory>

#include <cstddef>
#include <cstdint>

#include <uv.h>

namespace motion_monitor {

class observer
{
public:
  observer() = default;

  observer(const observer&) = default;

  observer(observer&&) = default;

  auto operator=(const observer&) -> observer& = delete;

  auto operator=(observer&&) -> observer& = delete;

  virtual ~observer() = default;

  virtual void on_error(const char* what) = 0;

  virtual void on_connection_established() = 0;

  virtual void on_connection_failed() = 0;

  virtual void on_connection_closed() = 0;

  virtual void on_image(std::size_t w, std::size_t h, std::size_t c, const std::uint8_t* data) = 0;
};

class connection
{
public:
  static auto create(uv_loop_t* loop) -> std::unique_ptr<connection>;

  connection() = default;

  virtual ~connection() = default;

  virtual void add_observer(observer* o) = 0;

  virtual void connect(const char* ip, int port) = 0;

  virtual void close() = 0;
};

} // namespace motion_monitor

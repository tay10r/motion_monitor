#pragma once

#include <memory>
#include <string>

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

  virtual void on_payload(const std::string& type, const void* payload, std::size_t payload_size) = 0;
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

  /**
   * @brief Sets whether or not to stream data from the server.
   *
   * @details By default, the connection class will automatically notify the server that the client is ready for data
   * whenever the observer class returns from its call. This can be disabled in order to manually control when the
   * server is allowed to send data again.
   *
   * @note This is on by default.
   * */
  virtual void set_streaming_enabled(bool enabled) = 0;

  /**
   * @brief Notifies the server that the client is ready for more data.
   *
   * @note This only has to be called if streaming is disabled.
   * */
  virtual void notify_ready() = 0;
};

} // namespace motion_monitor

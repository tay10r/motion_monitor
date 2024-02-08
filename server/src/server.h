#pragma once

#include <memory>

#include <uv.h>

#include <cstdint>

struct image;

class server
{
public:
  static auto create(uv_loop_t* loop) -> std::unique_ptr<server>;

  server() = default;

  server(const server&) = default;

  server(server&&) = default;

  auto operator=(const server&) -> server& = delete;

  auto operator=(server&&) -> server& = delete;

  virtual ~server() = default;

  virtual void close() = 0;

  virtual auto setup(const char* ip, int port) -> bool = 0;

  virtual void publish_telemetry(const void* data, std::size_t size) = 0;

  virtual void publish_frame(const image& img, const std::uint32_t sensor_id, const float anomaly_level) = 0;
};

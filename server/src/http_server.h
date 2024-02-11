#pragma once

#include <sentinel/proto.h>

#include <memory>
#include <string>
#include <vector>

#include <cstdint>
#include <cstring>

#include <uv.h>

struct image;

class http_server
{
public:
  static auto create(uv_loop_t* loop) -> std::unique_ptr<http_server>;

  http_server() = default;

  http_server(const http_server&) = default;

  http_server(http_server&&) = default;

  auto operator=(const http_server&) -> http_server& = delete;

  auto operator=(http_server&&) -> http_server& = delete;

  virtual ~http_server() = default;

  virtual void close() = 0;

  virtual auto setup(const char* ip, int port) -> bool = 0;

  virtual void add_file(std::string path, std::string content_type, std::vector<std::uint8_t> data) = 0;

  void add_file(std::string path, std::string content_type, std::string data)
  {
    std::vector<std::uint8_t> tmp(data.size(), 0);

    std::memcpy(&tmp[0], data.data(), data.size());

    add_file(path, content_type, std::move(tmp));
  }

  virtual void publish_camera_update(const image& img, std::uint32_t sensor_id, const float anomaly_level) = 0;

  virtual void publish_telemetry(std::shared_ptr<sentinel::proto::outbound_message>& msg) = 0;
};

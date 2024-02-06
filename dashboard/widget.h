#pragma once

#include <string>

class widget
{
public:
  virtual ~widget() = default;

  virtual void render() = 0;

  virtual void handle_telemetry(const std::string& type, const void* payload, std::size_t size) = 0;
};

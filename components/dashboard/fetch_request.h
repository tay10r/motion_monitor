#pragma once

#include <memory>

#include <cstddef>

class fetch_request
{
public:
  static auto create(const char* path) -> std::unique_ptr<fetch_request>;

  virtual ~fetch_request() = default;

  virtual void iterate() = 0;

  virtual auto done() const -> bool = 0;

  virtual auto success() const -> bool = 0;

  virtual auto get_data() const -> const void* = 0;

  virtual auto get_size() const -> std::size_t = 0;
};

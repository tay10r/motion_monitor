#pragma once

#include <vector>

#include <cstdint>

class pipeline
{
public:
  using buffer = std::vector<std::uint8_t>;

  using buffer_vec = std::vector<buffer>;

  pipeline() = default;

  virtual ~pipeline() = default;

  /**
   * @brief Iterates the pipeline.
   *
   * @param should_close May be set by the pipeline, in which case it will be closed before the next iteration.
   * */
  virtual auto loop(bool& should_close) -> buffer_vec = 0;
};

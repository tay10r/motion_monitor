#pragma once

#include <chrono>

class stop_watch final
{
public:
  using clock = std::chrono::high_resolution_clock;

  using time_point = typename clock::time_point;

  stop_watch()
    : m_last_time(clock::now())
  {
  }

  auto get_elapsed_ms() const -> std::chrono::milliseconds
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - m_last_time);
  }

  void reset() { m_last_time = clock::now(); }

private:
  time_point m_last_time;
};

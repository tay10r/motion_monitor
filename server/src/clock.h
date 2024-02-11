#pragma once

#include <chrono>

inline auto
get_clock_time() -> std::uint64_t
{
  using namespace std::chrono;

  return time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count();
}

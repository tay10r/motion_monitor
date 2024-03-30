#pragma once

#include <algorithm>
#include <chrono>

namespace sentinel {

inline auto
get_clock_time() -> std::uint64_t
{
  using namespace std::chrono;

  return time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count();
}

inline auto
get_time_difference(const std::uint64_t t0, const std::uint64_t t1) -> double
{
  const auto t_min = std::min(t0, t1);
  const auto t_max = std::max(t0, t1);

  const auto dt = t_max - t_min;

  return static_cast<double>(dt) * 1.0e-6;
}

} // namespace sentinel

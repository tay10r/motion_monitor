#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Used for observing telemetry data.
 * */
class telemetry_observer
{
public:
  virtual ~telemetry_observer() = default;

  /**
   * @brief Called when telemetry is produced by an entity, such as a sensor pipeline or remote peer.
   *
   * @param data The buffer containing the telemetry data.
   *
   * @param size The size of the telemetry buffer.
   * */
  virtual void observe_telemetry(const std::uint8_t* data, std::size_t size) = 0;
};

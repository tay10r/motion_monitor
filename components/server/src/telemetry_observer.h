#pragma once

#include <sentinel/proto.h>

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
   * @param msg The message to observe.
   * */
  virtual void observe_telemetry(std::shared_ptr<sentinel::proto::outbound_message>& msg) = 0;
};

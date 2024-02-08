#pragma once

#include "pipeline.h"

#include <vector>

#include <cstddef>
#include <cstdint>

class pipeline_base : public pipeline
{
public:
protected:
  void notify_telemetry(const std::uint8_t* data, std::size_t size);

private:
  std::vector<telemetry_observer*> m_observers;
};


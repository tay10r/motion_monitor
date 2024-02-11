#pragma once

#include "widget.h"

#include "config.h"

#include <memory>

class microphone_widget : public widget
{
public:
  static auto create(const config::microphone_widget_config& cfg) -> std::unique_ptr<microphone_widget>;

  virtual ~microphone_widget() = default;
};

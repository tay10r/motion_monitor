#pragma once

#include "widget.h"

#include "config.h"

#include <memory>

class camera_widget : public widget
{
public:
  static auto create(const config::camera_widget_config& cfg) -> std::unique_ptr<camera_widget>;

  virtual ~camera_widget() = default;
};

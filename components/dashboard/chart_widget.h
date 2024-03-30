#pragma once

#include "widget.h"

#include "config.h"

#include <memory>

class chart_widget : public widget
{
public:
  static auto create(const config::chart_widget_config& cfg) -> std::unique_ptr<chart_widget>;

  virtual ~chart_widget() = default;
};

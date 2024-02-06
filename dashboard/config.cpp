#include "config.h"

#include <nlohmann/json.hpp>

namespace {

void
load_widget_config(const nlohmann::json& root, config::widget_config& widget)
{
  widget.label = root["label"].get<std::string>();
  widget.row = root.at("row").get<int>();
  widget.col = root.at("col").get<int>();
  widget.row_span = root.at("row_span").get<int>();
  widget.col_span = root.at("col_span").get<int>();
}

auto
load_ui_config(const nlohmann::json& root) -> config::ui_config
{
  const auto grid = root["grid"];

  config::ui_config cfg;

  cfg.grid_rows = grid["rows"].get<int>();
  cfg.grid_cols = grid["cols"].get<int>();

  for (const auto& camera_cfg : root.at("camera_widgets")) {

    config::camera_widget_config widget;

    widget.sensor_id = camera_cfg.at("sensor_id").get<std::uint32_t>();

    load_widget_config(camera_cfg, widget);

    cfg.cameras.emplace_back(std::move(widget));
  }

  for (const auto& chart_cfg : root.at("chart_widgets")) {

    config::chart_widget_config widget;

    load_widget_config(chart_cfg, widget);

    widget.max_history = chart_cfg["max_history"].get<std::size_t>();
    widget.y_label = chart_cfg["y_label"].get<std::string>();

    for (const auto& id : chart_cfg["sensor_ids"]) {
      widget.sensor_ids.emplace_back(id.get<std::uint32_t>());
    }

    cfg.charts.emplace_back(std::move(widget));
  }

  return cfg;
}

} // namespace

void
config::parse(const void* data, const std::size_t size)
{
  const auto root = nlohmann::json::parse(static_cast<const char*>(data), static_cast<const char*>(data) + size);

  landscape_ui = load_ui_config(root["landscape_ui"]);

  portrait_ui = load_ui_config(root["portrait_ui"]);
}

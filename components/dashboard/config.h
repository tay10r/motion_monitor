#pragma once

#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

struct config final
{
  struct widget_config
  {
    std::string label;

    int row{ 0 };

    int col{ 0 };

    int row_span{ 1 };

    int col_span{ 1 };
  };

  struct camera_widget_config final : public widget_config
  {
    int sensor_id{};
  };

  struct microphone_widget_config final : public widget_config
  {
    int sensor_id{};
  };

  struct chart_widget_config final : public widget_config
  {
    std::size_t max_history{ 1000 };

    std::string y_label{ "Value" };

    std::vector<std::uint32_t> sensor_ids;
  };

  struct ui_config final
  {
    int grid_rows{ 3 };

    int grid_cols{ 3 };

    std::vector<camera_widget_config> cameras;

    std::vector<microphone_widget_config> microphones;

    std::vector<chart_widget_config> charts;
  };

  ui_config landscape_ui;

  ui_config portrait_ui;

  void parse(const void* data, std::size_t size);
};

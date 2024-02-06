#pragma once

#include <string>
#include <vector>

#include <cstdint>

struct config final
{
  struct camera_config final
  {
    /**
     * @brief The human-readable name to assign this camera.
     * */
    std::string name;

    /**
     * @brief The unique ID assigned to this sensor.
     * */
    std::uint32_t sensor_id{};

    /**
     * @brief The index of the device to open.
     * */
    int device_index{};

    /**
     * @brief The number of milliseconds between frame grabs.
     * */
    int read_interval{ 100 };

    /**
     * @brief The path to the detector model.
     * */
    std::string detector_path;
  };

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
    std::uint32_t sensor_id{};
  };

  struct ui_config final
  {
    int grid_rows{ 3 };

    int grid_cols{ 3 };

    std::vector<camera_widget_config> camera_widgets;
  };

  std::vector<camera_config> cameras;

  std::string server_ip{ "127.0.0.1" };

  int tcp_server_port{ 5100 };

  int http_server_port{ 8100 };

  bool tcp_server_enabled{ true };

  bool http_server_enabled{ true };

  ui_config landscape_ui;

  ui_config portrait_ui;

  void load(const char* path);

  auto export_dashboard_config() const -> std::string;
};

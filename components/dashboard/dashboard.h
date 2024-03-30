#pragma once

#include "config.h"
#include "widget.h"

#include <memory>
#include <string>
#include <vector>

#include <imgui.h>

class dashboard final
{
public:
  dashboard() = default;

  dashboard(const config::ui_config& cfg);

  void add_widget(const char* name, int r, int c, int rows, int cols, std::unique_ptr<widget> w);

  void render(const ImVec2& size);

  void handle_telemetry(const std::string& type, const void* payload, const std::size_t size);

protected:
  struct container final
  {
    std::string name;

    int row{ 0 };
    int col{ 0 };

    int row_span{ 1 };
    int col_span{ 1 };

    std::unique_ptr<widget> widget_instance;
  };

  auto get_cell_pos(const ImVec2& size, int row, int col) const -> ImVec2;

private:
  std::vector<container> m_containers;

  int m_grid_cols{ 1 };

  int m_grid_rows{ 1 };

  int m_padding{ 16 };
};

#include "dashboard.h"

#include "camera_widget.h"
#include "chart_widget.h"
#include "microphone_widget.h"

dashboard::dashboard(const config::ui_config& cfg)
  : m_grid_rows(cfg.grid_rows)
  , m_grid_cols(cfg.grid_cols)
{
  for (const auto& camera_cfg : cfg.cameras) {

    add_widget(camera_cfg.label.c_str(),
               camera_cfg.row,
               camera_cfg.col,
               camera_cfg.row_span,
               camera_cfg.col_span,
               camera_widget::create(camera_cfg));
  }

  for (const auto& mic_cfg : cfg.microphones) {

    add_widget(mic_cfg.label.c_str(),
               mic_cfg.row,
               mic_cfg.col,
               mic_cfg.row_span,
               mic_cfg.col_span,
               microphone_widget::create(mic_cfg));
  }

  for (const auto& chart_cfg : cfg.charts) {

    add_widget(chart_cfg.label.c_str(),
               chart_cfg.row,
               chart_cfg.col,
               chart_cfg.row_span,
               chart_cfg.col_span,
               chart_widget::create(chart_cfg));
  }
}

void
dashboard::add_widget(const char* name, int r, int c, int rows, int cols, std::unique_ptr<widget> w)
{
  container ctr{ name, r, c, rows, cols, std::move(w) };

  m_containers.emplace_back(std::move(ctr));
}

void
dashboard::render(const ImVec2& size)
{
  for (auto& c : m_containers) {

    const auto min_p = get_cell_pos(size, c.row, c.col);
    const auto max_p = get_cell_pos(size, c.row + c.row_span, c.col + c.col_span);

    const auto pos = ImVec2(min_p.x + m_padding, min_p.y + m_padding);

    const auto size = ImVec2((max_p.x - min_p.x) - m_padding * 2, (max_p.y - min_p.y) - m_padding * 2);

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

    ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    if (ImGui::Begin(
          c.name.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {

      c.widget_instance->render();
    }

    ImGui::End();
  }
}

void
dashboard::handle_telemetry(const std::string& type, const void* payload, const std::size_t size)
{
  for (auto& c : m_containers) {
    c.widget_instance->handle_telemetry(type, payload, size);
  }
}

auto
dashboard::get_cell_pos(const ImVec2& size, int row, int col) const -> ImVec2
{
  return ImVec2((size.x / m_grid_cols) * col, (size.y / m_grid_rows) * row);
}

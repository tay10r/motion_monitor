#include "chart_widget.h"

#include "telemetry_stream.h"

#include <sentinel/proto.h>

#include <implot.h>

namespace {

class chart_widget_impl final
  : public chart_widget
  , public sentinel::proto::payload_visitor
{
public:
  chart_widget_impl(const config::chart_widget_config& cfg)
  {
    m_x_label = "Time";

    m_y_label = "Temperature [F]";
    m_y_min = 0;
    m_y_max = 100;
  }

  void render() override
  {
    if (!ImPlot::BeginPlot("##Chart", ImVec2(-1, -1), ImPlotFlags_Crosshairs)) {
      return;
    }

    ImPlot::SetupAxis(ImAxis_X1, m_x_label.c_str(), ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxis(ImAxis_Y1, m_y_label.c_str());

    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
    ImPlot::SetupAxisLimits(ImAxis_Y1, m_y_min, m_y_max);

    ImPlot::PlotLine("##Temperature", m_x_history.data(), m_y_history.data(), m_x_history.size());

    ImPlot::EndPlot();
  }

  void handle_telemetry(const std::string& type, const void* payload, const std::size_t payload_size) override
  {
    sentinel::proto::decode_payload(type, payload, payload_size, *this);
  }

protected:
  void visit_rgb_camera_update(const std::uint8_t*, std::uint16_t, std::uint16_t, std::uint64_t, std::uint32_t) override
  {
  }

  void visit_monochrome_camera_update(const std::uint8_t*,
                                      std::uint16_t,
                                      std::uint16_t,
                                      std::uint64_t,
                                      std::uint32_t) override
  {
  }

  void visit_microphone_update(const std::int16_t*, std::uint32_t, std::uint32_t, std::uint64_t, std::uint32_t) override
  {
    //
  }

  void visit_temperature_update(const float temperature, const std::uint64_t time, std::uint32_t id) override
  {
    if (m_x_history.size() >= m_max_history) {
      m_x_history.erase(m_x_history.begin());
      m_y_history.erase(m_y_history.begin());
    }

    m_x_history.emplace_back(time * 1.0e-6);

    m_y_history.emplace_back(temperature);
  }

  auto visit_unknown_payload(const std::string& type, const void* payload, const std::size_t payload_size)
    -> bool override
  {
    return false;
  }

private:
  std::string m_x_label;

  std::string m_y_label;

  std::vector<double> m_x_history;

  std::vector<double> m_y_history;

  const std::size_t m_max_history{ 1000 };

  double m_y_min{ 0 };

  double m_y_max{ 1 };
};

} // namespace

auto
chart_widget::create(const config::chart_widget_config& cfg) -> std::unique_ptr<chart_widget>
{
  return std::make_unique<chart_widget_impl>(cfg);
}

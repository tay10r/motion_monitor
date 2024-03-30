#include "microphone_widget.h"

#include "telemetry_stream.h"

#include <sentinel/proto.h>

#include <implot.h>

#include <limits>

#include <cmath>

namespace {

class microphone_widget_impl final
  : public microphone_widget
  , public sentinel::proto::payload_visitor_base
{
public:
  microphone_widget_impl(const config::microphone_widget_config& cfg)
    : m_config(cfg)
  {
  }

  void render() override
  {
    if (!ImPlot::BeginPlot("##Chart", ImVec2(-1, -1), ImPlotFlags_Crosshairs)) {
      return;
    }

    ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_AutoFit);

    ImPlot::SetupAxis(ImAxis_Y1, "Sound Level");

    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);

    ImPlot::PlotLine("##SoundLevel", m_x_history.data(), m_y_history.data(), m_x_history.size());

    ImPlot::EndPlot();
  }

  void handle_telemetry(const std::string& type, const void* payload, const std::size_t payload_size) override
  {
    sentinel::proto::decode_payload(type, payload, payload_size, *this);
  }

protected:
  void visit_microphone_update(const std::int16_t* data,
                               std::uint32_t size,
                               std::uint32_t sample_rate,
                               std::uint64_t time,
                               std::uint32_t sensor_id) override
  {
    constexpr auto scale = -1.0f / static_cast<float>(std::numeric_limits<std::int16_t>::min());

    float sum{};

    for (std::size_t i = 0; i < size; i++) {

      const auto val = static_cast<float>(data[i]) * scale;

      sum += std::abs(val);
    }

    add_chart_entry(time * 1.0e-6, sum / static_cast<float>(size));
  }

  void add_chart_entry(const double x, const double y)
  {
    if (m_x_history.size() >= m_max_history) {
      m_x_history.erase(m_x_history.begin());
      m_y_history.erase(m_y_history.begin());
    }

    m_x_history.emplace_back(x);

    m_y_history.emplace_back(y);
  }

private:
  std::vector<double> m_x_history;

  std::vector<double> m_y_history;

  const std::size_t m_max_history{ 8192 };

  config::microphone_widget_config m_config;
};

} // namespace

auto
microphone_widget::create(const config::microphone_widget_config& cfg) -> std::unique_ptr<microphone_widget>
{
  return std::make_unique<microphone_widget_impl>(cfg);
}

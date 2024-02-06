#include <uikit/fonts.hpp>
#include <uikit/main.hpp>

#include <motion_monitor_proto.h>

#include "config.h"
#include "dashboard.h"
#include "fetch_request.h"

#include "camera_widget.h"
#include "chart_widget.h"

namespace {

class app_impl final : public uikit::app
{
public:
  void setup(uikit::platform& plt) override
  {
    plt.set_auto_close_enabled(true);

    m_config_fetch_request = fetch_request::create("config.json");

    ImGui::GetStyle().WindowTitleAlign.x = 0.5f;
  }

  void teardown(uikit::platform&) override
  {
    //
  }

  void loop(uikit::platform& plt) override
  {
    if (m_config_fetch_request) {
      poll_config_fetch_request();
    }

    if (m_telemetry_fetch_request) {
      poll_telemetry_fetch_request();
    }

    const auto& io = ImGui::GetIO();

    {
      const auto size = io.DisplaySize;
      ImGui::SetNextWindowSize(size, ImGuiCond_Always);
      ImGui::SetNextWindowPos(ImVec2(0, 0));
    }

    render_main_window();
  }

protected:
  void build_dashboard()
  {
    m_landscape_dashboard = dashboard(m_config.landscape_ui);

    m_portrait_dashboard = dashboard(m_config.portrait_ui);
  }

  void render_main_window()
  {
    const auto display_size = ImGui::GetIO().DisplaySize;

    if (display_size.x >= display_size.y) {
      m_landscape_dashboard.render(ImGui::GetIO().DisplaySize);
    } else {
      m_portrait_dashboard.render(ImGui::GetIO().DisplaySize);
    }
  }

  void poll_config_fetch_request()
  {
    m_config_fetch_request->iterate();

    if (!m_config_fetch_request->done()) {
      return;
    }

    if (m_config_fetch_request->success()) {

      m_config.parse(m_config_fetch_request->get_data(), m_config_fetch_request->get_size());

      build_dashboard();

      begin_telemetry_polling();
    }

    m_config_fetch_request.reset();
  }

  void begin_telemetry_polling() { m_telemetry_fetch_request = fetch_request::create("api/stream"); }

  void poll_telemetry_fetch_request()
  {
    m_telemetry_fetch_request->iterate();

    if (!m_telemetry_fetch_request->done()) {
      return;
    }

    if (m_telemetry_fetch_request->success()) {

      const auto* data = static_cast<const std::uint8_t*>(m_telemetry_fetch_request->get_data());

      const auto result = motion_monitor::read(data, m_telemetry_fetch_request->get_size());

      if (result.payload_ready) {

        m_landscape_dashboard.handle_telemetry(result.type_id, data + result.payload_offset, result.payload_size);
      }
    }

    begin_telemetry_polling();
  }

private:
  std::unique_ptr<fetch_request> m_config_fetch_request;

  std::unique_ptr<fetch_request> m_telemetry_fetch_request;

  config m_config;

  dashboard m_landscape_dashboard;

  dashboard m_portrait_dashboard;
};

} // namespace

namespace uikit {

auto
app::create() -> std::unique_ptr<app>
{
  return std::unique_ptr<app>(new app_impl());
}

} // namespace uikit

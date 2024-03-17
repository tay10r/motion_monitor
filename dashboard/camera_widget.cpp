#include "camera_widget.h"

#include "stop_watch.h"
#include "telemetry_stream.h"

#include <sentinel/proto.h>

#include <GLES2/gl2.h>

#include <imgui.h>

#include <implot.h>

#include <chrono>
#include <sstream>

namespace {

class camera_widget_impl final
  : public camera_widget
  , public sentinel::proto::payload_visitor_base
{
public:
  explicit camera_widget_impl(const config::camera_widget_config& cfg)
  {
    glGenTextures(1, &m_texture);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, "\x00\x00\x00\x00");
  }

  ~camera_widget_impl() { glDeleteTextures(1, &m_texture); }

  void render() override
  {
    const auto text_size = ImGui::CalcTextSize(m_time_string.data(), m_time_string.data() + m_time_string.size()).x;

    const auto window_size = ImGui::GetWindowSize().x;

    ImGui::SetCursorPosX((window_size - text_size) * 0.5f);

    ImGui::TextUnformatted(m_time_string.data(), m_time_string.data() + m_time_string.size());

    if (ImPlot::BeginPlot("##View", ImVec2(-1, -1), ImPlotFlags_Equal)) {

      ImPlot::SetupAxis(ImAxis_X1, "", ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoDecorations);
      ImPlot::SetupAxis(ImAxis_Y1, "", ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoDecorations);

      ImPlot::PlotImage(
        "##LatestFrame", reinterpret_cast<ImTextureID>(m_texture), ImPlotPoint(0, 0), ImPlotPoint(1, 1));

      ImPlot::EndPlot();
    }
  }

  void handle_telemetry(const std::string& type_id, const void* payload, const std::size_t payload_size) override
  {
    sentinel::proto::decode_payload(type_id, payload, payload_size, *this);
  }

protected:
  void visit_rgb_camera_frame_event(const sentinel::proto::camera_frame_event& ev) override
  {
    std::vector<std::uint8_t> rgba(ev.w * ev.h * 4, 0);

    for (std::size_t i = 0; i < (static_cast<std::size_t>(ev.w) * ev.h); i++) {

      rgba[i * 4 + 0] = ev.data[i * 3 + 0];
      rgba[i * 4 + 1] = ev.data[i * 3 + 1];
      rgba[i * 4 + 2] = ev.data[i * 3 + 2];
      rgba[i * 4 + 3] = 255;
    }

    update_image(rgba.data(), ev.w, ev.h, ev.time);
  }

  void visit_monochrome_camera_frame_event(const sentinel::proto::camera_frame_event& ev) override
  {
    std::vector<std::uint8_t> rgba(ev.w * ev.h * 4);

    for (std::size_t i = 0; i < (static_cast<std::size_t>(ev.w) * ev.h); i++) {
      rgba[i * 4 + 0] = ev.data[i];
      rgba[i * 4 + 1] = ev.data[i];
      rgba[i * 4 + 2] = ev.data[i];
      rgba[i * 4 + 3] = 255;
    }

    update_image(rgba.data(), ev.w, ev.h, ev.time);
  }

protected:
  void update_image(const std::uint8_t* rgba, std::uint16_t w, std::uint16_t h, std::uint64_t t)
  {
    using namespace std::chrono;

    const time_point<system_clock> p{ duration_cast<system_clock::time_point::duration>(microseconds(t)) };

    const auto p_tmp = system_clock::to_time_t(p);

    std::ostringstream time_stream;

    time_stream << std::ctime(&p_tmp);

    m_time_string = time_stream.str();

    m_width = w;

    m_height = h;

    m_last_time = t;

    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  }

private:
  GLuint m_texture{};

  GLsizei m_width{};

  GLsizei m_height{};

  std::uint64_t m_last_time{ 0 };

  std::string m_time_string;
};

} // namespace

auto
camera_widget::create(const config::camera_widget_config& cfg) -> std::unique_ptr<camera_widget>
{
  return std::unique_ptr<camera_widget>(new camera_widget_impl(cfg));
}

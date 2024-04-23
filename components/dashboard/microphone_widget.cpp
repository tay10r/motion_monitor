#include "microphone_widget.h"

#include <sentinel/proto.h>

#include <implot.h>

#include <AL/al.h>
#include <AL/alc.h>

#include <limits>
#include <vector>

#include <cmath>

#include <iostream>
#include <sstream>

namespace {

class audio_player final
{
public:
  audio_player(const audio_player&) = delete;

  audio_player(audio_player&&) = delete;

  auto operator=(const audio_player&) -> audio_player& = delete;

  auto operator=(audio_player&&) -> audio_player& = delete;

  audio_player()
  {
    m_available_buffers.resize(32);

    alGenBuffers(m_available_buffers.size(), &m_available_buffers[0]);

    alGenSources(1, &m_source);

    alSourcef(m_source, AL_PITCH, 1);
    alSourcef(m_source, AL_GAIN, 1);
    alSource3f(m_source, AL_POSITION, 0, 0, 0);
    alSource3f(m_source, AL_VELOCITY, 0, 0, 0);
    alSourcei(m_source, AL_LOOPING, AL_FALSE);
  }

  ~audio_player()
  {
    alSourceStop(m_source);

    alDeleteSources(1, &m_source);

    alDeleteBuffers(m_available_buffers.size(), m_available_buffers.data());
  }

  void update_playback_state(const bool state)
  {
    if (state) {
      alSourcePlay(m_source);
    } else {
      alSourceStop(m_source);
    }
  }

  void queue_buffer(const std::int16_t* data, std::uint32_t size, std::uint32_t sample_rate)
  {
    std::ostringstream log;

    log << "queue_buffer\n";

    ALint processed_buffers{};
    alGetSourceiv(m_source, AL_BUFFERS_PROCESSED, &processed_buffers);

    log << "  processed=" << processed_buffers << '\n';

    while (processed_buffers > 0) {
      ALuint buf{};
      alSourceUnqueueBuffers(m_source, 1, &buf);
      m_available_buffers.emplace_back(buf);
      log << "  unqueue=" << static_cast<int>(buf) << '\n';
      processed_buffers--;
    }

    log << "  available=" << static_cast<int>(m_available_buffers.size()) << '\n';

    if (m_available_buffers.size() > 0) {

      auto buf = m_available_buffers.at(0);
      m_available_buffers.erase(m_available_buffers.begin());
      alBufferData(buf, AL_FORMAT_MONO16, data, size, sample_rate);
      alSourceQueueBuffers(m_source, 1, &buf);
      log << "  using=" << static_cast<int>(buf) << "\n";

      ALint state{};
      alGetSourceiv(m_source, AL_SOURCE_STATE, &state);
      if ((state == AL_STOPPED) || (state == AL_PAUSED)) {
        log << "  restarting\n";
        alSourcePlay(m_source);
      }
    }

    std::cout << log.str() << std::endl;
  }

private:
  bool m_state{ false };

  ALuint m_source{};

  std::vector<ALuint> m_available_buffers;
};

class microphone_widget_impl final
  : public microphone_widget
  , public sentinel::proto::payload_visitor_base
{
public:
  explicit microphone_widget_impl(const config::microphone_widget_config& cfg)
    : m_config(cfg)
  {
  }

  void render() override
  {
    if (ImGui::Checkbox("Listen", &m_listen)) {
      m_player.update_playback_state(m_listen);
    }

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
    if (m_config.sensor_id != sensor_id) {
      return;
    }

    constexpr auto scale = -1.0f / static_cast<float>(std::numeric_limits<std::int16_t>::min());

    float max{};

    for (std::size_t i = 0; i < size; i++) {

      const auto val = static_cast<float>(data[i]) * scale;

      max = std::max(std::abs(val), max);
    }

    add_chart_entry(time * 1.0e-6, max);

    if (m_listen) {
      m_player.queue_buffer(data, size, sample_rate);
    }
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

  bool m_listen{ false };

  audio_player m_player;
};

} // namespace

auto
microphone_widget::create(const config::microphone_widget_config& cfg) -> std::unique_ptr<microphone_widget>
{
  return std::make_unique<microphone_widget_impl>(cfg);
}

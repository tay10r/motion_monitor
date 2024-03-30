#include "microphone_device.h"

#include <alsa/asoundlib.h>

namespace {

class microphone_device_impl final : public microphone_device
{
public:
  explicit microphone_device_impl(const char* device, const unsigned int sampling_rate)
    : m_sampling_rate(sampling_rate)
  {
    if (snd_pcm_open(&m_handle, device, SND_PCM_STREAM_CAPTURE, 0) < 0) {
      return;
    }

    snd_pcm_hw_params_t* hw_params{ nullptr };

    if (snd_pcm_hw_params_malloc(&hw_params) < 0) {
      snd_pcm_close(m_handle);
      return;
    }

    snd_pcm_hw_params_any(m_handle, hw_params);

    snd_pcm_hw_params_get_period_size(hw_params, &m_period_size, nullptr);

    if (snd_pcm_hw_params_any(m_handle, hw_params) < 0) {
      snd_pcm_hw_params_free(hw_params);
      snd_pcm_close(m_handle);
      return;
    }

    if (snd_pcm_hw_params_set_access(m_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
      snd_pcm_hw_params_free(hw_params);
      snd_pcm_close(m_handle);
      return;
    }

    const snd_pcm_format_t format{ SND_PCM_FORMAT_S16_LE };

    if (snd_pcm_hw_params_set_format(m_handle, hw_params, format) < 0) {
      snd_pcm_hw_params_free(hw_params);
      snd_pcm_close(m_handle);
      return;
    }

    if (snd_pcm_hw_params_set_rate_near(m_handle, hw_params, &m_sampling_rate, 0) < 0) {
      snd_pcm_hw_params_free(hw_params);
      snd_pcm_close(m_handle);
      return;
    }

    if (snd_pcm_hw_params_set_channels(m_handle, hw_params, 1) < 0) {
      snd_pcm_hw_params_free(hw_params);
      snd_pcm_close(m_handle);
      return;
    }

    if (snd_pcm_hw_params(m_handle, hw_params) < 0) {
      snd_pcm_hw_params_free(hw_params);
      snd_pcm_close(m_handle);
      return;
    }

    snd_pcm_hw_params_free(hw_params);

    if (snd_pcm_prepare(m_handle) < 0) {
      snd_pcm_close(m_handle);
      return;
    }

    m_open = true;
  }

  ~microphone_device_impl()
  {
    if (m_open) {
      snd_pcm_close(m_handle);
    }
  }

  auto is_open() const -> bool override { return m_open; }

  auto get_rate() const -> std::uint32_t override { return m_sampling_rate; }

  auto read() -> std::vector<std::int16_t> override
  {
    std::vector<std::int16_t> buffer(m_period_size);

    const auto read_size = snd_pcm_readi(m_handle, &buffer[0], m_period_size);
    if (read_size < 0) {
      return buffer;
    }

    buffer.resize(static_cast<std::size_t>(read_size));

    return buffer;
  }

private:
  snd_pcm_t* m_handle{ nullptr };

  bool m_open{ false };

  unsigned int m_sampling_rate{ 44100 };

  snd_pcm_uframes_t m_period_size{ 1024 };
};

} // namespace

auto
microphone_device::create(const char* device, const unsigned int sampling_rate) -> std::unique_ptr<microphone_device>
{
  return std::make_unique<microphone_device_impl>(device, sampling_rate);
}

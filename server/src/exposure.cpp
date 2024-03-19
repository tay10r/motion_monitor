#include "exposure.h"

#include "image.h"
#include "video_device.h"

#include <spdlog/spdlog.h>

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <array>

#include <iostream>

namespace {

class gradient_maximization_exposure final : public exposure
{
public:
  // TODO : Nelder–Mead optimizer

  struct estimator final
  {
    std::array<float, 3> scores{ 0, 0, 0 };

    std::array<int, 3> samples{ 0, 0, 0 };

    int idx{};

    int sample_idx{};
  };

  void update(video_device& dev, const image& img) override
  {
    if (!m_initialized) {

      dev.set_manual_exposure_enabled(true);

      m_current = dev.get_exposure();

      m_initialized = true;
    }

    cv::Mat h_grad;
    cv::Sobel(img.frame, h_grad, CV_8U, 1, 0);

    cv::Mat v_grad;
    cv::Sobel(img.frame, v_grad, CV_8U, 0, 1);

    cv::Mat grad;
    cv::addWeighted(h_grad, 0.5, v_grad, 0.5, 0, grad);

    m_estimator.scores[m_estimator.idx] += cv::sum(grad)[0];

    m_estimator.samples[m_estimator.idx]++;

    if (m_estimator.samples[m_estimator.idx] == m_num_samples) {

      m_estimator.idx++;

      if (m_estimator.idx == 3) {

        complete_iteration(dev);

      } else {

        const auto delta = m_delta * m_deltas[m_estimator.idx];

        dev.set_exposure(get_exposure(delta));
      }
    }
  }

protected:
  void complete_iteration(video_device& dev)
  {
    std::size_t best_idx{};

    for (std::size_t i = 1; i < 3; i++) {
      if (m_estimator.scores[i] > m_estimator.scores[best_idx]) {
        best_idx = i;
      }
    }

    m_current = get_exposure(m_deltas[best_idx] * m_delta);

    dev.set_exposure(m_current);

    std::cout << "setting to " << m_current << std::endl;

    m_estimator = estimator{};
  }

  auto get_exposure(float delta) const -> float { return std::min(std::max(m_current + delta, 1.0f), 5000.0f); }

private:
  float m_current{ 1000 };

  float m_delta{ 500 };

  int m_num_samples{ 10 };

  const std::array<int, 3> m_deltas{ 0, -1, 1 };

  estimator m_estimator;

  bool m_initialized{ false };
};

template<bool ManualEnabled>
class camera_exposure_setting final : public exposure
{
public:
  void update(video_device& dev, const image& img) override
  {
    if (!m_set) {

      spdlog::info("Setting manual exposure mode to {}.", ManualEnabled);

      dev.set_manual_exposure_enabled(ManualEnabled);

      m_set = true;
    }
  }

private:
  bool m_set{ false };
};

class null_exposure final : public exposure
{
public:
  void update(video_device&, const image&) override {}
};

} // namespace

auto
exposure::create(const config::camera_exposure_mode mode) -> std::unique_ptr<exposure>
{
  switch (mode) {
    case config::camera_exposure_mode::none:
      break;
    case config::camera_exposure_mode::manual:
      return std::make_unique<camera_exposure_setting<true>>();
    case config::camera_exposure_mode::automatic:
      return std::make_unique<camera_exposure_setting<false>>();
    case config::camera_exposure_mode::gradient_maximization:
      return std::make_unique<gradient_maximization_exposure>();
  }

  return std::make_unique<null_exposure>();
}

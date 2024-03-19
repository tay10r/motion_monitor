#include "video_device.h"

#include <opencv2/videoio.hpp>

#include "clock.h"
#include "image.h"

#include <random>
#include <vector>

namespace {

class video_device_impl final : public video_device
{
public:
  auto open(int device_index, int frame_w, int frame_h) -> bool override
  {
    m_handle.open(device_index, cv::CAP_V4L2);

    m_frame_width = frame_w;

    m_frame_height = frame_h;

    if (m_handle.isOpened()) {

      m_handle.set(cv::CAP_PROP_FRAME_WIDTH, frame_w);

      m_handle.set(cv::CAP_PROP_FRAME_HEIGHT, frame_h);

      m_frame_width = m_handle.get(cv::CAP_PROP_FRAME_WIDTH);

      m_frame_height = m_handle.get(cv::CAP_PROP_FRAME_HEIGHT);
    }

    return m_handle.isOpened();
  }

  auto read_frame() -> image override
  {
    if (!m_handle.isOpened()) {
      return create_bad_image();
    }

    cv::Mat frame;

    m_handle.read(frame);

    const auto t = get_clock_time();

    image img;

    img.time = t;

    img.resize(frame.cols, frame.rows, 3);

    for (std::size_t i = 0; i < (frame.cols * frame.rows); i++) {

      const auto x = i % frame.cols;
      const auto y = i / frame.cols;

      const auto pixel = frame.at<cv::Vec3b>(y, x);

      img.data.at(i * 3 + 0) = pixel[2];
      img.data.at(i * 3 + 1) = pixel[1];
      img.data.at(i * 3 + 2) = pixel[0];
    }

    img.frame = std::move(frame);

    return img;
  }

  void set_manual_exposure_enabled(bool enabled) override
  {
    // These are magic values based on the V4L2 API.
    //
    // They might be camera specific, not entirely sure.
    constexpr int manual = 1;
    constexpr int aperture_priority = 3;
    m_handle.set(cv::CAP_PROP_AUTO_EXPOSURE, enabled ? manual : aperture_priority);
  }

  virtual void set_exposure(float exposure) override { m_handle.set(cv::CAP_PROP_EXPOSURE, exposure); }

  virtual auto get_exposure() const -> float override { return m_handle.get(cv::CAP_PROP_EXPOSURE); }

protected:
  auto create_bad_image() -> image
  {
    const int w = m_frame_width;

    const int h = m_frame_height;

    image img;

    img.time = get_clock_time();

    img.resize(w, h, 3);

    std::uniform_int_distribution<int> dist(0, 255);

    for (int i = 0; i < (w * h); i++) {

      const auto r = dist(m_rng);
      const auto g = dist(m_rng);
      const auto b = dist(m_rng);

      img.data[i * 3 + 0] = r;
      img.data[i * 3 + 1] = g;
      img.data[i * 3 + 2] = b;
      img.frame.at<cv::Vec3b>(i) = cv::Vec3b(b, g, r);
    }

    return img;
  }

private:
  cv::VideoCapture m_handle;

  std::mt19937 m_rng{ 0 };

  int m_frame_width{ 640 };

  int m_frame_height{ 480 };
};

} // namespace

auto
video_device::create() -> std::unique_ptr<video_device>
{
  return std::make_unique<video_device_impl>();
}

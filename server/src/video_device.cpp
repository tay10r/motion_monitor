#include "video_device.h"

#ifndef FAKE_VIDEO_DEVICE
#include <opencv2/videoio.hpp>
#else
#include <random>
#endif

#include "image.h"

#include <vector>

namespace {

#ifndef FAKE_VIDEO_DEVICE

class video_device_impl final : public video_device
{
public:
  auto open(int device_index) -> bool override
  {
    m_handle.open(device_index, cv::CAP_V4L2);

    return m_handle.isOpened();
  }

  auto read_frame() -> image override
  {
    cv::Mat data;

    m_handle.read(data);

    image img;

    img.resize(data.cols, data.rows, 3);

    for (std::size_t i = 0; i < (data.cols * data.rows); i++) {

      const auto x = i % data.cols;
      const auto y = i / data.cols;

      const auto pixel = data.at<cv::Vec3b>(y, x);

      img.data.at(i * 3 + 0) = pixel[0];
      img.data.at(i * 3 + 1) = pixel[1];
      img.data.at(i * 3 + 2) = pixel[2];
    }

    return img;
  }

private:
  cv::VideoCapture m_handle;
};

#else /* FAKE_VIDEO_DEVICE */

class video_device_impl final : public video_device
{
public:
  auto open(int device_index) -> bool override
  {
    std::seed_seq seed{ device_index };

    m_rng = std::mt19937(seed);

    return true;
  }

  auto read_frame() -> image override
  {
    const std::size_t w{ 256 };
    const std::size_t h{ 256 };

    image img;

    img.resize(w, h, 3);

    std::uniform_int_distribution<int> dist(0, 255);

    for (std::size_t i = 0; i < (w * h); i++) {
      img.data[i * 3 + 0] = dist(m_rng);
      img.data[i * 3 + 1] = dist(m_rng);
      img.data[i * 3 + 2] = dist(m_rng);
    }

    return img;
  }

private:
  std::mt19937 m_rng;
};

#endif /* FAKE_VIDEO_DEVICE */

} // namespace

auto
video_device::create() -> std::unique_ptr<video_device>
{
  return std::make_unique<video_device_impl>();
}

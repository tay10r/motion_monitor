#include "video_device.h"

#include <opencv2/videoio.hpp>

#include "image.h"

#include <random>
#include <vector>

namespace {

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
    if (!m_handle.isOpened()) {
      return create_bad_image();
    }

    cv::Mat data;

    m_handle.read(data);

    cv::Mat rgb_data;

    data.convertTo(rgb_data, CV_8UC3);

    image img;

    img.resize(rgb_data.cols, rgb_data.rows, 3);

    for (std::size_t i = 0; i < (rgb_data.cols * rgb_data.rows); i++) {

      const auto x = i % rgb_data.cols;
      const auto y = i / rgb_data.cols;

      const auto pixel = rgb_data.at<cv::Vec3b>(y, x);

      img.data.at(i * 3 + 0) = pixel[0];
      img.data.at(i * 3 + 1) = pixel[1];
      img.data.at(i * 3 + 2) = pixel[2];
    }

    return img;
  }

protected:
  auto create_bad_image() -> image
  {
    const int w = 640;
    const int h = 480;

    image img;

    img.resize(w, h, 3);

    std::uniform_int_distribution<int> dist(0, 255);

    for (int i = 0; i < (w * h); i++) {
      img.data[i * 3 + 0] = dist(m_rng);
      img.data[i * 3 + 1] = dist(m_rng);
      img.data[i * 3 + 2] = dist(m_rng);
    }

    return img;
  }

private:
  cv::VideoCapture m_handle;
  std::mt19937 m_rng{ 0 };
};

} // namespace

auto
video_device::create() -> std::unique_ptr<video_device>
{
  return std::make_unique<video_device_impl>();
}

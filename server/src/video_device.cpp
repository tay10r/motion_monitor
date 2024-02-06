#include "video_device.h"

#include <opencv2/videoio.hpp>

#include "image.h"

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

private:
  cv::VideoCapture m_handle;
};

} // namespace

auto
video_device::create() -> std::unique_ptr<video_device>
{
  return std::make_unique<video_device_impl>();
}

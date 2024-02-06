#include "detector.h"

#include "image.h"

#include <opencv2/dnn/dnn.hpp>

#include <spdlog/spdlog.h>

namespace {

class detector_impl final : public detector
{
public:
  explicit detector_impl(const char* path)
    : m_net(cv::dnn::readNetFromONNX(path))
  {
    //
  }

  auto exec(const image& img) -> float override
  {
    std::vector<std::uint8_t> tmp_data = img.data;

    cv::Mat tmp(img.height, img.width, CV_8UC3, tmp_data.data());

    const auto input = cv::dnn::blobFromImage(tmp);

    m_net.setInput(input);

    const auto output = m_net.forward();

    const auto err = cv::norm(output - input);

    spdlog::info("inference error: {}", err);
    //

    return 1.0f;
  }

private:
  cv::dnn::Net m_net;
};

} // namespace

auto
detector::create(const char* path) -> std::unique_ptr<detector>
{
  return std::make_unique<detector_impl>(path);
}

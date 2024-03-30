#include "video_frame_filter.h"

#include "clock.h"
#include "image.h"

#include <opencv2/dnn.hpp>

#include <optional>

namespace {

class video_frame_filter_impl final : public video_frame_filter
{
public:
  explicit video_frame_filter_impl(const std::string& model_path,
                                   const std::size_t output_index,
                                   const bool apply_sigmoid,
                                   const double max_time)
    : m_network(cv::dnn::readNet(model_path))
    , m_output_index(output_index)
    , m_apply_sigmoid(apply_sigmoid)
    , m_max_time(max_time)
  {
  }

  auto filter(const image& input) -> bool
  {
    auto blob = cv::dnn::blobFromImage(input.frame);

    m_network.setInput(blob, "", 1.0f / 256.0f);

    std::vector<cv::Mat> outputs;

    m_network.forward(outputs);

    auto output = outputs.at(m_output_index).at<float>(0);

    if (m_apply_sigmoid) {
      output = 1.0f / (1.0f + std::exp(-output));
    }

    auto image_class = output >= 0.5f;

    if (m_max_time >= 0.0) {

      if (m_last_frame_time.has_value()) {
        const auto dt = sentinel::get_time_difference(m_last_frame_time.value(), input.time);
        if (dt > m_max_time) {
          image_class = true;
        }
      }

      if (image_class) {
        m_last_frame_time = input.time;
      }
    }

    return image_class;
  }

private:
  cv::dnn::Net m_network;

  const std::size_t m_output_index{ 0 };

  const bool m_apply_sigmoid{ true };

  std::optional<std::uint64_t> m_last_frame_time;

  const double m_max_time{ -1 };
};

} // namespace

auto
video_frame_filter::create(const std::string& model_path,
                           const std::size_t output_index,
                           const bool apply_sigmoid,
                           const double max_time) -> std::unique_ptr<video_frame_filter>
{
  return std::make_unique<video_frame_filter_impl>(model_path, output_index, apply_sigmoid, max_time);
}

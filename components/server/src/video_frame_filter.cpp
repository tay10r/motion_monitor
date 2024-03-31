#include "video_frame_filter.h"

#include "clock.h"
#include "image.h"

#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

#include <optional>

namespace {

class video_frame_filter_impl final : public video_frame_filter
{
public:
  explicit video_frame_filter_impl(const std::string& model_path,
                                   const std::size_t output_index,
                                   const bool apply_sigmoid,
                                   const double threshold,
                                   const double max_time,
                                   const int input_w,
                                   const int input_h,
                                   const bool input_grayscale)
    : m_network(cv::dnn::readNet(model_path))
    , m_output_index(output_index)
    , m_apply_sigmoid(apply_sigmoid)
    , m_threshold(threshold)
    , m_max_time(max_time)
    , m_input_w(input_w)
    , m_input_h(input_h)
    , m_input_grayscale(input_grayscale)
  {
  }

  auto filter(const image& input) -> bool
  {
    int input_w{ m_input_w };
    int input_h{ m_input_h };

    if (input_w == -1) {
      input_w = input.width;
    }

    if (input_h == -1) {
      input_h = input.height;
    }

    cv::Mat frame = input.frame;

    if ((input_w != input.width) || (input_h != input.height)) {
      cv::Mat tmp;
      cv::resize(input.frame, tmp, cv::Size(input_w, input_h));
      frame = std::move(tmp);
    }

    if (m_input_grayscale) {
      cv::Mat tmp;
      cv::cvtColor(frame, tmp, cv::COLOR_BGR2GRAY);
      frame = std::move(tmp);
    }

    auto blob = cv::dnn::blobFromImage(frame, 1.0 / 256.0f);

    m_network.setInput(blob);

    std::vector<cv::Mat> outputs;

    m_network.forward(outputs);

    auto output = outputs.at(m_output_index).at<float>(0);

    if (m_apply_sigmoid) {
      output = 1.0f / (1.0f + std::exp(-output));
    }

    auto image_class = output >= static_cast<float>(m_threshold);

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

  const double m_threshold{ 0.5 };

  const int m_input_w{ -1 };

  const int m_input_h{ -1 };

  const bool m_input_grayscale{ false };
};

} // namespace

auto
video_frame_filter::create(const std::string& model_path,
                           const std::size_t output_index,
                           const bool apply_sigmoid,
                           const double threshold,
                           const double max_time,
                           const int input_w,
                           const int input_h,
                           const bool input_grayscale) -> std::unique_ptr<video_frame_filter>
{
  return std::make_unique<video_frame_filter_impl>(
    model_path, output_index, apply_sigmoid, threshold, max_time, input_w, input_h, input_grayscale);
}

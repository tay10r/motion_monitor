#include "image.h"

#include <opencv2/opencv.hpp>

#include <stdexcept>

void
image::resize(std::size_t w, std::size_t h, std::size_t c)
{
  width = w;
  height = h;
  channels = c;
  data.resize(w * h * c);

  if (frame.empty()) {
    switch (c) {
      case 1:
        frame = cv::Mat(h, w, CV_8UC1);
        break;
      case 2:
        frame = cv::Mat(h, w, CV_8UC2);
        break;
      case 3:
        frame = cv::Mat(h, w, CV_8UC3);
        break;
      case 4:
        frame = cv::Mat(h, w, CV_8UC4);
        break;
      default:
        throw std::runtime_error("Invalid number of channels.");
    }
  } else {
    cv::Mat next_frame;
    cv::resize(frame, next_frame, cv::Size(w, h));
    frame = std::move(next_frame);
  }
}

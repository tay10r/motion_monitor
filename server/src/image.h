#pragma once

#include <opencv2/core/mat.hpp>

#include <vector>

#include <cstdint>

struct image final
{
  std::size_t width{};

  std::size_t height{};

  std::size_t channels{};

  /**
   * @brief The RGB form of the data.
   *
   * @note This field is retained for compatibility.
   *       New code should use @ref image::frame instead.
   * */
  std::vector<std::uint8_t> data;

  /**
   * @brief The frame produced by the video device.
   *
   * @note This field obsoletes the @ref image::data field.
   * */
  cv::Mat frame;

  /**
   * @brief The time at which the frame was grabbed, in terms of microseconds since Unix epoch (local time).
   * */
  std::uint64_t time{};

  void resize(std::size_t w, std::size_t h, std::size_t c);
};

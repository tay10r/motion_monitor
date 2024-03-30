#pragma once

#include <memory>

struct image;

/**
 * @brief This class is used for filtering out frames that may not be of interest to the rest of the system.
 *
 * @note The reason this component may be used is to either save storage cost or minimize the amount of bandwidth when
 *       streaming to other system components.
 * */
class video_frame_filter
{
public:
  /**
   * @brief Creates a new video frame filter.
   *
   * @param model_path The path of the model to filter images.
   *
   * @param output_index The index of the output containing the binary classification.
   *
   * @param apply_sigmoid Whether or not to apply sigmoid to the model output.
   *
   * @param max_time The maximum amount of time the filter is allowed to reject images, in terms of seconds.
   *
   * @return A new video frame filter.
   * */
  static auto create(const std::string& model_path, std::size_t output_index, bool apply_sigmoid, double max_time)
    -> std::unique_ptr<video_frame_filter>;

  virtual ~video_frame_filter() = default;

  /**
   * @param input The input image to filter.
   *
   * @return True if the image should be passed through the rest of the system, false otherwise.
   * */
  virtual auto filter(const image& input) -> bool = 0;
};

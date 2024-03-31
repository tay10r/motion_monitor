#pragma once

#include <string>
#include <vector>

#include <cstdint>

struct config final
{
  struct camera_config final
  {
    /**
     * @brief The human-readable name to assign this camera.
     * */
    std::string name;

    /**
     * @brief The unique ID assigned to this sensor.
     * */
    std::uint32_t sensor_id{};

    /**
     * @brief The index of the device to open.
     * */
    int device_index{};

    /**
     * @brief The width of each frame, in terms of pixels.
     * */
    int frame_width{ 640 };

    /**
     * @brief The height of each frame, in terms of pixels.
     * */
    int frame_height{ 480 };

    /**
     * @brief The quality of JPEG to produce - a trade off between bandwidth and image quality.
     * */
    float jpeg_quality{ 0.5f };

    /**
     * @brief Whether or not to enable the HOG-SVM people detector.
     * */
    bool people_detection_enabled{ false };

    /**
     * @brief Whether or not to store camera frames.
     * */
    bool storage_enabled{ true };

    /**
     * @brief At what quality to store the camera frames.
     * */
    float storage_quality{ 0.5f };

    /**
     * @brief Where to store the camera frames.
     * */
    std::string storage_path{ "." };

    /**
     * @brief The number of days to store each frame.
     * */
    float storage_days{ 7.0f };

    /**
     * @brief The width of each frame, when put into storage.
     * */
    int storage_width{ -1 };

    /**
     * @brief The height of each frame, when put into storage.
     * */
    int storage_height{ -1 };

    /**
     * @brief The number of seconds between storing frames.
     * */
    float storage_rate{ 1.0f };

    /**
     * @brief Whether or not to enable frame filtering.
     * */
    bool frame_filter_enabled{ false };

    /**
     * @brief The path to the model to use when filtering frames.
     * */
    std::string frame_filter_model_path;

    /**
     * @brief The index of the filter model output containing binary classification.
     * */
    std::size_t frame_filter_output_index{ 0 };

    /**
     * @brief Whether or not to apply the sigmoid function to the filter output.
     * */
    bool frame_filter_apply_sigmoid{ true };

    /**
     * @brief The maximum amount of time that the filter is allowed to reject frames.
     * */
    double frame_filter_max_time{ 900 /* 15 minutes */ };

    /**
     * @brief The threshold to consider the filter output as a positive sample.
     * */
    double frame_filter_threshold{ 0.5 };

    /**
     * @brief The width to resize the frame before feeding it to the frame filter.
     *        Negative one means no change.
     * */
    int frame_filter_input_width{ -1 };

    /**
     * @brief The height to resize the frame before feeding it to the frame filter.
     *        Negative one means no change.
     * */
    int frame_filter_input_height{ -1 };

    /**
     * @brief Whether or not to convert the image to grayscale.
     * */
    bool frame_filter_input_grayscale{ false };
  };

  struct microphone_config final
  {
    /**
     * @brief The name of the device (which must be unique).
     * */
    std::string name;

    /**
     * @brief The unique ID assigned to this microphone.
     * */
    std::uint32_t sensor_id{};

    /**
     * @brief The sampling rate of the audio.
     * */
    unsigned int rate{ 44100 };
  };

  struct widget_config
  {
    std::string label;

    int row{ 0 };

    int col{ 0 };

    int row_span{ 1 };

    int col_span{ 1 };
  };

  struct camera_widget_config final : public widget_config
  {
    std::uint32_t sensor_id{};
  };

  struct microphone_widget_config final : public widget_config
  {
    std::uint32_t sensor_id{};
  };

  struct ui_config final
  {
    int grid_rows{ 3 };

    int grid_cols{ 3 };

    std::vector<camera_widget_config> camera_widgets;

    std::vector<microphone_widget_config> microphone_widgets;
  };

  std::vector<camera_config> cameras;

  std::vector<microphone_config> microphones;

  std::string server_ip{ "127.0.0.1" };

  int tcp_server_port{ 5100 };

  int http_server_port{ 8100 };

  bool tcp_server_enabled{ true };

  bool http_server_enabled{ true };

  ui_config landscape_ui;

  ui_config portrait_ui;

  void load(const char* path);

  void load_string(const char* str);

  /**
   * @brief Validates the configuration.
   *
   * @note An exception is thrown if the configuration contains an error.
   * */
  void validate() const;

  auto export_dashboard_config() const -> std::string;
};

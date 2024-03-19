#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <cstdint>

struct config final
{
  enum class camera_exposure_mode
  {
    /**
     * @brief The exposure mode is not controlled in this case.
     * */
    none,

    /**
     * @brief The exposure is manually controlled.
     * */
    manual,

    /**
     * @brief The camera's auto exposure is used.
     * */
    automatic,

    /**
     * @brief The exposure level is optimized to increase the gradient magnitude of the image.
     * */
    gradient_maximization
  };

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
     * @brief Used for deciding how to control the exposure.
     * */
    camera_exposure_mode exposure_mode{ camera_exposure_mode::none };

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

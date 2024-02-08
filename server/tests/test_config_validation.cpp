#include <gtest/gtest.h>

#include "../src/config.h"

#include <stdexcept>

TEST(Config, ValidateDuplicateCameraNames)
{
  const char* config_str = R"(
  cameras:
    - name: 'camera 0'
      device_index: 0
    - name: 'camera 0'
      device_index: 1
  )";

  config cfg;

  cfg.load_string(config_str);

  EXPECT_THROW(cfg.validate(), std::runtime_error);
}

TEST(Config, ValidateDuplicateMicrophoneNames)
{
  const char* config_str = R"(
  microphones:
    - name: 'mic 0'
    - name: 'mic 0'
  )";

  config cfg;

  cfg.load_string(config_str);

  EXPECT_THROW(cfg.validate(), std::runtime_error);
}

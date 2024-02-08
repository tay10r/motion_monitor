#include "src/camera_pipeline.h"
#include "src/config.h"
#include "src/http_server.h"
#include "src/image.h"
#include "src/microphone_pipeline.h"
#include "src/pipeline_runner.h"
#include "src/server.h"
#include "src/video_device.h"

#include <spdlog/spdlog.h>

#include <iostream>
#include <string>
#include <vector>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifdef WITH_BUNDLE
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(motion_monitor_resources);
#endif

namespace {

auto
to_handle(uv_timer_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

auto
to_handle(uv_signal_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

#ifdef WITH_BUNDLE
auto
open_rc_file(const char* path) -> std::vector<std::uint8_t>
{
  const auto fs = cmrc::motion_monitor_resources::get_filesystem();

  const auto file = fs.open(path);

  std::vector<std::uint8_t> data;

  data.resize(file.size());

  std::memcpy(&data[0], file.begin(), file.size());

  return data;
}
#endif /* WITH_BUNDLE */

class program final
  : public camera_pipeline::observer
  , public telemetry_observer
{
public:
  program()
  {
    uv_loop_init(&m_loop);

    uv_handle_set_data(to_handle(&m_signal), this);
    uv_signal_init(&m_loop, &m_signal);

    m_server = server::create(&m_loop);

    m_http_server = http_server::create(&m_loop);

#ifdef WITH_BUNDLE
    m_http_server->add_file("/index.html", "text/html", open_rc_file("index.html"));
    m_http_server->add_file("/dashboard.js", "text/javascript", open_rc_file("dashboard.js"));
    m_http_server->add_file("/dashboard.wasm", "application/wasm", open_rc_file("dashboard.wasm"));
#endif
  }

  program(const program&) = delete;

  program(program&&) = delete;

  auto operator=(const program&) -> program& = delete;

  auto operator=(program&&) -> program& = delete;

  ~program() { uv_loop_close(&m_loop); }

  void run(const config& cfg)
  {
    m_http_server->add_file("/config.json", "application/json", cfg.export_dashboard_config());

    for (const auto& camera_cfg : cfg.cameras) {

      auto p = camera_pipeline::create(&m_loop, camera_cfg);

      p->add_observer(this);

      m_cameras.emplace_back(std::move(p));
    }

    for (const auto& microphone_cfg : cfg.microphones) {

      auto p = microphone_pipeline::create(microphone_cfg);

      auto runner = std::make_unique<pipeline_runner>(&m_loop, std::move(p));

      runner->add_telemetry_observer(this);

      m_pipeline_runners.emplace_back(std::move(runner));
    }

    uv_signal_start(&m_signal, on_signal, SIGINT);

    if (cfg.tcp_server_enabled) {
      m_server->setup(cfg.server_ip.c_str(), cfg.tcp_server_port);
    }

    if (cfg.http_server_enabled) {
      m_http_server->setup(cfg.server_ip.c_str(), cfg.http_server_port);
    }

    spdlog::info("Starting IO loop.");

    uv_run(&m_loop, UV_RUN_DEFAULT);

    shutdown();
  }

protected:
  static auto get_self(uv_handle_t* handle) -> program* { return static_cast<program*>(uv_handle_get_data(handle)); }

  static void on_signal(uv_signal_t* signal, int signum)
  {
    spdlog::info("Caught signal, stopping loop.");

    auto* self = get_self(to_handle(signal));

    uv_stop(&self->m_loop);
  }

  void observe_telemetry(const std::uint8_t* data, const std::size_t size) override
  {
    spdlog::info("got telemetry.");

    m_server->publish_telemetry(data, size);
  }

  void on_image_update(const image& img, const std::uint32_t sensor_id, const float anomaly_level) override
  {
    spdlog::info("Publishing new image update.");

    m_server->publish_frame(img, sensor_id, anomaly_level);

    m_http_server->publish_camera_update(img, sensor_id, anomaly_level);
  }

  void shutdown()
  {
    m_server->close();

    m_http_server->close();

    for (auto& cam : m_cameras) {
      cam->close();
    }

    uv_signal_stop(&m_signal);

    uv_close(to_handle(&m_signal), nullptr);

    for (auto& r : m_pipeline_runners) {
      r->close();
    }

    uv_run(&m_loop, UV_RUN_DEFAULT);
  }

private:
  uv_loop_t m_loop{};

  uv_signal_t m_signal{};

  std::unique_ptr<server> m_server;

  std::unique_ptr<http_server> m_http_server;

  std::vector<std::unique_ptr<camera_pipeline>> m_cameras;

  std::vector<std::unique_ptr<microphone_pipeline>> m_microphones;

  std::vector<std::unique_ptr<pipeline_runner>> m_pipeline_runners;
};

const char help[] = R"(
Options:
  --config PATH : Specifies the path to the configuration file.
  --help        : The device index to capture video from.
)";

} // namespace

int
main(int argc, char** argv)
{
  config cfg;

  int port{ 5100 };

  int http_port{ 8100 };

  int device_index{ 0 };

  const char* config_path{ "config.yaml" };

  for (int i = 1; i < argc; i++) {
    const auto has_next = (i + 1) < argc;
    if (std::strcmp(argv[i], "--config") == 0) {
      if (!has_next) {
        std::cerr << "Missing path to '--config' argument." << std::endl;
        return EXIT_FAILURE;
      }
      config_path = argv[i + 1];
      i++;
    } else if (std::strcmp(argv[i], "--help") == 0) {
      std::cerr << "Usage: " << argv[i] << " [options]" << std::endl;
      std::cerr << help;
      return EXIT_FAILURE;
    } else {
      std::cerr << "Invalid argument '" << argv[i] << "' (see --help)." << std::endl;
      return EXIT_FAILURE;
    }
  }

  cfg.load(config_path);

  try {
    cfg.validate();
  } catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  {
    auto prg = std::make_unique<program>();

    prg->run(cfg);
  }

  return EXIT_SUCCESS;
}

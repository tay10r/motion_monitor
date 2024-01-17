#include "src/image.h"
#include "src/motion_filter.h"
#include "src/server.h"
#include "src/video_device.h"

#include <spdlog/spdlog.h>

#include <iostream>

#include <cstdlib>
#include <cstring>

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

class program final
{
public:
  program()
  {
    uv_loop_init(&m_loop);

    uv_handle_set_data(to_handle(&m_timer), this);
    uv_timer_init(&m_loop, &m_timer);

    uv_handle_set_data(to_handle(&m_signal), this);
    uv_signal_init(&m_loop, &m_signal);

    m_server = server::create(&m_loop);

    m_video_device = video_device::create();
  }

  program(const program&) = delete;

  program(program&&) = delete;

  auto operator=(const program&) -> program& = delete;

  auto operator=(program&&) -> program& = delete;

  ~program() { uv_loop_close(&m_loop); }

  void run(const char* ip, const int port, int device_index)
  {
    if (!m_video_device->open(device_index)) {
      spdlog::error("Failed to open video capture device.");
      return;
    }

    uv_timer_start(&m_timer, on_timer_expire, 0, 100);

    uv_signal_start(&m_signal, on_signal, SIGINT);

    m_server->setup(ip, port);

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

  static void on_timer_expire(uv_timer_t* timer)
  {
    spdlog::debug("Capturing frame.");

    auto* self = get_self(to_handle(timer));

    self->update_frame();
  }

  void update_frame()
  {
    m_current_frame = m_video_device->read_frame();

    m_current_frame_id++;

    const auto motion = m_motion_filter->filter(m_current_frame);

    m_server->publish_frame(m_current_frame, motion);
  }

  void shutdown()
  {
    m_server->close();

    uv_timer_stop(&m_timer);
    uv_close(to_handle(&m_timer), nullptr);

    uv_signal_stop(&m_signal);
    uv_close(to_handle(&m_signal), nullptr);

    uv_run(&m_loop, UV_RUN_DEFAULT);
  }

private:
  uv_loop_t m_loop{};

  uv_timer_t m_timer{};

  uv_signal_t m_signal{};

  std::unique_ptr<video_device> m_video_device;

  std::unique_ptr<server> m_server;

  image m_current_frame;

  std::size_t m_current_frame_id{};

  std::unique_ptr<motion_filter> m_motion_filter{ motion_filter::create() };
};

const char help[] = R"(
Options:
  --ip        IP : Specifies the IP address to bind the TCP server to.
  --port    PORT : The port to bind the TCP server to.
  --device INDEX : The device index to capture video from.
)";

} // namespace

int
main(int argc, char** argv)
{
  std::string ip{ "127.0.0.1" };

  int port{ 5100 };

  int device_index{ 0 };

  for (int i = 1; i < argc; i++) {
    const auto has_next = (i + 1) < argc;
    if (std::strcmp(argv[i], "--ip") == 0) {
      if (!has_next) {
        std::cerr << "Missing argument to IP argument." << std::endl;
        return EXIT_FAILURE;
      }
      ip = argv[i + 1];
      i++;
    } else if (std::strcmp(argv[i], "--port") == 0) {
      if (!has_next) {
        std::cerr << "Missing argument to --port" << std::endl;
        return EXIT_FAILURE;
      }
      char* endptr{ nullptr };
      port = static_cast<int>(std::strtol(argv[i + 1], &endptr, 10));
      if ((argv[i + 1] == endptr) || ((*endptr) != '\0')) {
        std::cerr << "Invalid port '" << argv[i + 1] << "'." << std::endl;
        return EXIT_FAILURE;
      }
      i++;
    } else if (std::strcmp(argv[i], "--device") == 0) {
      if (!has_next) {
        std::cerr << "Missing argument to --device" << std::endl;
        return EXIT_FAILURE;
      }
      char* endptr{ nullptr };
      device_index = static_cast<int>(std::strtol(argv[i + 1], &endptr, 10));
      if ((argv[i + 1] == endptr) || ((*endptr) != '\0')) {
        std::cerr << "Invalid device '" << argv[i + 1] << "'." << std::endl;
        return EXIT_FAILURE;
      }
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

  {
    auto prg = std::make_unique<program>();

    prg->run(ip.c_str(), port, device_index);
  }

  return EXIT_SUCCESS;
}

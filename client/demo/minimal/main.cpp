#include <motion_monitor.h>

#include <iostream>

#include <cstdlib>

namespace {

class observer_impl final : public motion_monitor::observer
{
public:
  void on_error(const char* what) override { std::cerr << what << std::endl; }

  void on_connection_established() override { std::cout << "connection established" << std::endl; }

  void on_connection_failed() override { std::cout << "connection failed" << std::endl; }

  void on_connection_closed() override { std::cout << "connection closed" << std::endl; }

  void on_image(std::size_t w, std::size_t h, std::size_t c, const std::uint8_t* data) override
  {
    std::cout << "on image" << std::endl;
    //
  }
};

} // namespace

int
main()
{
  uv_loop_t loop{};

  uv_loop_init(&loop);

  auto connection = motion_monitor::connection::create(&loop);

  observer_impl obs;

  connection->add_observer(&obs);

  connection->connect("127.0.0.1", 5100);

  uv_run(&loop, UV_RUN_DEFAULT);

  std::cout << "shutting down" << std::endl;

  connection->close();

  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);

  return EXIT_SUCCESS;
}

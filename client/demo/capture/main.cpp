#include <motion_monitor.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <cstdlib>

#include <stb_image_write.h>

namespace {

class observer_impl final : public motion_monitor::observer
{
public:
  explicit observer_impl(motion_monitor::connection* conn)
    : m_connection(conn)
  {
  }

  void on_error(const char* what) override { std::cerr << what << std::endl; }

  void on_connection_established() override { std::cout << "connection established" << std::endl; }

  void on_connection_failed() override { std::cout << "connection failed" << std::endl; }

  void on_connection_closed() override { std::cout << "connection closed" << std::endl; }

  void on_image(std::size_t w, std::size_t h, std::size_t c, const std::uint8_t* data) override
  {
    std::ostringstream path_stream;
    path_stream << std::setw(8) << std::setfill('0') << m_frame_counter << ".png";
    const auto path = path_stream.str();

    std::cout << "saving frame '" << path << "'" << std::endl;

    stbi_write_png(path.c_str(), w, h, c, data, w * c);

    m_frame_counter++;
  }

private:
  motion_monitor::connection* m_connection{ nullptr };

  int m_frame_counter{ 0 };
};

} // namespace

int
main(int argc, char** argv)
{
  const char* ip = "127.0.0.1";

  if (argc > 1) {
    ip = argv[1];
  }

  uv_loop_t loop{};

  uv_loop_init(&loop);

  auto connection = motion_monitor::connection::create(&loop);

  observer_impl obs(connection.get());

  connection->add_observer(&obs);

  connection->connect(ip, 5100);

  uv_run(&loop, UV_RUN_DEFAULT);

  std::cout << "shutting down" << std::endl;

  connection->close();

  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);

  return EXIT_SUCCESS;
}

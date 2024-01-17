#include <uikit/main.hpp>

#include <GLES2/gl2.h>

#include <motion_monitor.h>

#include <iostream>

#include <algorithm>

#include <cstdlib>

namespace {

class app_impl final
  : public motion_monitor::observer
  , public uikit::app
{
public:
  void setup(uikit::platform& plt) override
  {
    uv_loop_init(&m_loop);

    m_connection = motion_monitor::connection::create(&m_loop);

    m_connection->add_observer(this);

    m_connection->connect("10.0.0.154", 5100);
    //
    // m_connection->connect("127.0.0.1", 5100);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    unsigned char buffer[4]{};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  }

  void teardown(uikit::platform& plt) override
  {
    m_connection->close();

    uv_run(&m_loop, UV_RUN_DEFAULT);

    uv_loop_close(&m_loop);

    glDeleteTextures(1, &m_texture);
  }

  void loop(uikit::platform& plt) override
  {
    uv_run(&m_loop, UV_RUN_NOWAIT);

    ImGui::DockSpaceOverViewport();

    if (ImGui::Begin("Viewer")) {
      ImGui::Image(reinterpret_cast<ImTextureID>(m_texture), m_texture_size);
    }

    ImGui::End();
  }

  void on_error(const char* what) override { std::cerr << what << std::endl; }

  void on_connection_established() override { std::cout << "connection established" << std::endl; }

  void on_connection_failed() override { std::cout << "connection failed" << std::endl; }

  void on_connection_closed() override { std::cout << "connection closed" << std::endl; }

  void on_image(std::size_t w, std::size_t h, std::size_t c, const std::uint8_t* data) override
  {
    std::vector<unsigned char> rgba(w * h * 4, 255);

    for (std::size_t i = 0; i < (w * h); i++) {

      for (std::size_t j = 0; j < std::min(c, static_cast<std::size_t>(4)); j++) {

        rgba[i * 4 + j] = data[i * c + j];
      }
    }

    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    m_texture_size = ImVec2(w, h);
  }

private:
  uv_loop_t m_loop{};

  std::unique_ptr<motion_monitor::connection> m_connection;

  GLuint m_texture{};

  ImVec2 m_texture_size{ 1, 1 };
};

} // namespace

namespace uikit {

auto
app::create() -> std::unique_ptr<app>
{
  return std::unique_ptr<uikit::app>(new app_impl());
}

} // namespace uikit

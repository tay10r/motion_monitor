#include <uikit/main.hpp>

#include <glad/glad.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>

#include <stb_image.h>

namespace uikit {

namespace {

struct entry final
{
  std::filesystem::path path;

  int label{ 0 };
};

struct current_image final
{
  GLuint texture{};

  int selected{ -1 };

  int width{};

  int height{};

  current_image()
  {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  current_image(const current_image&) = delete;

  void destroy() { glDeleteTextures(1, &texture); }

  void load(int index, const entry& ent)
  {
    int w = 0;
    int h = 0;
    auto* data = stbi_load(ent.path.string().c_str(), &w, &h, nullptr, 3);
    if (data != nullptr) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      width = w;
      height = h;
      stbi_image_free(data);
    }
  }
};

auto
operator<(const entry& a, const entry& b) -> bool
{
  return a.path < b.path;
}

class app_impl final : public app
{
public:
  void setup(uikit::platform&) override
  {
    for (const auto& img : std::filesystem::directory_iterator("unlabeled")) {

      const std::string ext = img.path().extension().string();

      if ((ext == ".png") || (ext == ".jpg")) {
        m_unlabeled.emplace_back(entry{ img.path(), 0 });
      }
    }

    std::sort(m_unlabeled.begin(), m_unlabeled.end());

    for (const auto& img : std::filesystem::directory_iterator("labeled/0")) {
      m_class_counts[0]++;
    }

    for (const auto& img : std::filesystem::directory_iterator("labeled/1")) {
      m_class_counts[1]++;
    }
  }

  void loop(uikit::platform&) override
  {
    ImGui::DockSpaceOverViewport();

    if (ImGui::Begin("Image")) {

      const auto max_selected = m_unlabeled.empty() ? 0 : static_cast<int>(m_unlabeled.size() - 1);

      ImGui::SliderInt("Selected", &m_selected, 0, max_selected);

      const auto in_bounds = (m_selected >= 0) && (m_selected < m_unlabeled.size());

      if (in_bounds) {

        if (ImGui::Button("Mark")) {
          m_unlabeled.at(m_selected).label = 1;
          m_selected++;
        }
      }
    }

    if ((m_selected >= 0) && (m_selected < static_cast<int>(m_unlabeled.size()))) {

      m_current_image.load(m_selected, m_unlabeled.at(m_selected));

      ImGui::Image(reinterpret_cast<ImTextureID>(m_current_image.texture),
                   ImVec2(m_current_image.width, m_current_image.height));
    }

    ImGui::End();
  }

  void teardown(uikit::platform&) override
  {
    std::array<std::vector<std::filesystem::path>, 2> outputs;

    for (const auto& entry : m_unlabeled) {
      outputs.at(entry.label).emplace_back(entry.path);
    }

    /* Make sure the images are equally distributed, randomly sample from the label with the most samples. */

    const int label_with_max = (outputs.at(0).size() > outputs.at(1).size()) ? 0 : 1;

    std::vector<std::size_t> indices(outputs.at(label_with_max).size());

    for (std::size_t i = 0; i < indices.size(); i++) {
      indices[i] = i;
    }

    std::seed_seq seed{ 0 };

    std::mt19937 rng(seed);

    for (std::size_t i = 1; i < indices.size(); i++) {

      std::uniform_int_distribution<std::size_t> dist(i, indices.size() - 1);

      indices[i - 1] = indices[dist(rng)];
    }

    {
      std::vector<std::filesystem::path> tmp(outputs.at((label_with_max + 1) % 2).size());

      for (std::size_t i = 0; i < tmp.size(); i++) {
        tmp[i] = outputs.at(label_with_max).at(indices.at(i));
      }

      outputs.at(label_with_max) = std::move(tmp);
    }

    /* Move to the respective directories. */

    for (std::size_t label_idx = 0; label_idx < 2; label_idx++) {

      for (const auto& entry : outputs.at(label_idx)) {

        std::filesystem::path output_path{ "labeled" };

        if (label_idx == 0) {
          output_path = output_path / "0";
        } else {
          output_path = output_path / "1";
        }

        std::ostringstream name_stream;
        name_stream << std::setw(8) << std::setfill('0') << m_class_counts.at(label_idx) << ".png";
        const auto name = name_stream.str();

        output_path = output_path / name;

        std::filesystem::rename(entry, output_path);

        m_class_counts.at(label_idx)++;
      }
    }

    m_current_image.destroy();
  }

private:
  std::vector<entry> m_unlabeled;

  int m_selected{ 0 };

  current_image m_current_image;

  std::array<int, 2> m_class_counts;
};

} // namespace

auto
app::create() -> std::unique_ptr<app>
{
  return std::make_unique<app_impl>();
}

} // namespace uikit

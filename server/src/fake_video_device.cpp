#include "video_device.h"

#include <random>
#include <vector>

#include <glm/glm.hpp>

#include "image.h"

namespace {

constexpr int spp = 4;

class video_device_impl final : public video_device
{
public:
  auto open(int device_index) -> bool override
  {
    std::seed_seq seed{ device_index };

    m_rng = std::mt19937(seed);

    return true;
  }

  auto read_frame() -> image override
  {
    const std::size_t w{ 512 };
    const std::size_t h{ 512 };

    image img;

    img.resize(w, h, 3);

    std::uniform_int_distribution<int> dist(0, 255);

    const auto ray_org = glm::vec3(0, 1, 0);

    for (std::size_t i = 0; i < (w * h); i++) {

      const auto x = i % w;
      const auto y = i / w;

      glm::vec3 color(0, 0, 0);

      constexpr float scale = 1.0f / static_cast<float>(spp);

      std::uniform_real_distribution<float> uv_dist(0, 1);

      for (int j = 0; j < spp; j++) {

        const float u = (static_cast<float>(x) + uv_dist(m_rng)) / static_cast<float>(w);
        const float v = (static_cast<float>(y) + uv_dist(m_rng)) / static_cast<float>(h);

        const float dx = u * 2.0f - 1.0f;
        const float dy = 1.0f - v * 2.0f;
        const float dz = -1.0f;

        const auto ray_dir = glm::normalize(glm::vec3(dx, dy, dz));

        color += trace(ray_org, ray_dir, m_rng);
      }

      img.data[i * 3 + 0] = 255 * color.r * scale;
      img.data[i * 3 + 1] = 255 * color.g * scale;
      img.data[i * 3 + 2] = 255 * color.b * scale;
    }

    return img;
  }

protected:
  template<typename Rng>
  auto trace(const glm::vec3& ray_org, const glm::vec3& ray_dir, Rng& rng, int depth = 0) -> glm::vec3
  {
    constexpr int max_depth = 3;

    if (depth >= max_depth) {
      return glm::vec3(0, 0, 0);
    }

    const auto plane_hit = intersect_plane(ray_org, ray_dir, { 0, 0, 0 }, { 0, -1, 0 });

    if (plane_hit >= 0.0f) {
      const auto hit_pos = ray_org + ray_dir * plane_hit;
      const auto x = static_cast<int>(hit_pos.x);
      const auto z = static_cast<int>(hit_pos.z);
      const auto albedo = ((z * 3 + x) % 2 == 0) ? glm::vec3(0.90f, 0.90f, 0.90f) : glm::vec3(0.85f, 0.85f, 0.85f);
      return albedo * trace(hit_pos, sample_hemisphere({ 0, 1, 0 }, rng), rng, depth + 1);
    }

    return on_miss(ray_dir);
  }

  template<typename Rng>
  auto sample_hemisphere(const glm::vec3& n, Rng& rng) -> glm::vec3
  {
    std::uniform_real_distribution<float> dist(-1, 1);

    glm::vec3 v{ 0, 0, 0 };

    while (true) {
      v.x = dist(rng);
      v.y = dist(rng);
      v.z = dist(rng);

      if ((glm::dot(v, v) < 1.0f) && (glm::dot(v, n) >= 0.0f)) {
        break;
      }
    }

    return glm::normalize(v);
  }

  static auto intersect_plane(const glm::vec3& ray_org,
                              const glm::vec3& ray_dir,
                              const glm::vec3& p,
                              const glm::vec3& n) -> float
  {
    const auto denom = glm::dot(n, ray_dir);
    if (denom < 1.0e-6f) {
      return -1;
    }

    const auto t = glm::dot(-ray_org, n) / denom;

    return t;
  }

  static auto on_miss(const glm::vec3& ray_dir) -> glm::vec3
  {
    const auto up = glm::vec3(0, 1, 0);
    const auto hi = glm::vec3(0.5, 0.7, 1.0);
    const auto lo = glm::vec3(1.0, 1.0, 1.0);
    const auto level = glm::dot(up, ray_dir);
    return lo + (hi - lo) * level;
  }

private:
  std::mt19937 m_rng;
};

} // namespace

auto
video_device::create() -> std::unique_ptr<video_device>
{
  return std::make_unique<video_device_impl>();
}

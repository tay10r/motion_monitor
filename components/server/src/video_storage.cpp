#include "video_storage.h"

#include "image.h"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <optional>
#include <sstream>

namespace {

template<typename Scalar>
auto
clamp(Scalar x, Scalar min, Scalar max) -> Scalar
{
  return std::max(std::min(x, max), min);
}

auto
get_max_dt(const float days) -> std::uint64_t
{
  if (days < 0) {
    return std::numeric_limits<std::uint64_t>::max();
  }

  return static_cast<std::uint64_t>(static_cast<double>(days) * 24 * 60 * 60 * 1000 * 1000);
}

class video_storage_impl final : public video_storage
{
public:
  explicit video_storage_impl(std::string path,
                              const float quality,
                              const float days,
                              const int storage_width,
                              const int storage_height,
                              const float rate)
    : m_path(std::move(path))
    , m_quality(quality)
    , m_days(days)
    , m_max_dt(get_max_dt(days))
    , m_storage_width(storage_width)
    , m_storage_height(storage_height)
    , m_rate(rate)
  {
    for (const auto entry : std::filesystem::directory_iterator(m_path)) {

      const auto entry_path = entry.path();

      if (entry_path.extension().string() != ".jpg") {
        continue;
      }

      std::istringstream in_stream(entry_path.stem().string());

      std::uint64_t timestamp{};

      if (!(in_stream >> timestamp)) {
        continue;
      }

      m_existing_paths.emplace_back(timestamp, entry_path.string());
    }

    using entry_type = std::pair<std::uint64_t, std::string>;

    auto cmp = [](const entry_type& l, const entry_type& r) -> bool { return l.first < r.first; };

    std::sort(m_existing_paths.begin(), m_existing_paths.end(), cmp);
  }

  void store(const image& img) override
  {
    if (img.frame.empty()) {
      return;
    }

    if (m_last_time.has_value()) {
      const auto last_t = m_last_time.value();
      const auto t = img.time;
      const auto elapsed = static_cast<double>(t - last_t) * 1.0e-6;
      if (elapsed < (1.0 / m_rate)) {
        return;
      }
    }

    std::ostringstream path_stream;
    if (!m_path.empty()) {
      path_stream << m_path << '/';
    }
    path_stream << img.time;
    path_stream << ".jpg";

    auto path = path_stream.str();

    if ((m_storage_width < 0) || (m_storage_height < 0)) {
      store(std::move(path), img.frame, img.time);
      m_last_time = img.time;
      return;
    }

    cv::Mat frame;
    cv::resize(img.frame, frame, cv::Size(m_storage_width, m_storage_height));
    store(std::move(path), frame, img.time);

    m_last_time = img.time;
  }

protected:
  void store(std::string path, const cv::Mat& frame, const std::uint64_t t)
  {
    const int jpeg_quality{ clamp<int>(static_cast<int>(m_quality * 100), 0, 100) };

    cv::imwrite(path, frame, { cv::IMWRITE_JPEG_QUALITY, jpeg_quality });

    m_existing_paths.emplace_back(t, std::move(path));

    remove_old_entries(t);
  }

  void remove_old_entries(const std::uint64_t last_frame_t)
  {
    while (!m_existing_paths.empty()) {

      auto it = m_existing_paths.begin();

      const auto frame_t = it->first;

      const auto dt = (frame_t > last_frame_t) ? static_cast<std::uint64_t>(0) : (last_frame_t - frame_t);

      if (dt > m_max_dt) {
        std::filesystem::remove(it->second);
        m_existing_paths.erase(it);
      } else {
        break;
      }
    }
  }

private:
  std::string m_path;

  const float m_quality{ 0.5f };

  const float m_days{ 7.0f };

  const std::uint64_t m_max_dt{};

  int m_storage_width{ -1 };

  int m_storage_height{ -1 };

  std::vector<std::pair<std::uint64_t, std::string>> m_existing_paths;

  float m_rate{ 1 };

  std::optional<std::uint64_t> m_last_time;
};

} // namespace

auto
video_storage::create(std::string path, float quality, float days, int storage_width, int storage_height, float rate)
  -> std::unique_ptr<video_storage>
{
  return std::make_unique<video_storage_impl>(std::move(path), quality, days, storage_width, storage_height, rate);
}

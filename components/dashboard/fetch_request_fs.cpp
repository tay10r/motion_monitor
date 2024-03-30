#include "fetch_request.h"

#include <fstream>
#include <string>

namespace {

class fetch_request_impl final : public fetch_request
{
public:
  fetch_request_impl(const char* path)
    : m_path(path)
  {
  }

  auto done() const -> bool override { return m_done; }

  auto success() const -> bool override { return m_success; }

  auto get_data() const -> const void* override { return m_data.data(); }

  auto get_size() const -> std::size_t override { return m_data.size(); }

  void iterate() override
  {
    if (m_done) {
      return;
    }

    m_done = true;

    std::ifstream file(m_path);

    if (!file.good()) {
      return;
    }

    file.seekg(0, std::ios::end);

    const auto file_size = file.tellg();
    if (file_size == -1l) {
      return;
    }

    file.seekg(0, std::ios::beg);

    m_data.resize(file_size);

    file.read(&m_data[0], m_data.size());

    m_success = true;
  }

private:
  std::string m_path;

  bool m_done{ false };

  bool m_success{ false };

  std::string m_data;
};

} // namespace

auto
fetch_request::create(const char* path) -> std::unique_ptr<fetch_request>
{
  return std::make_unique<fetch_request_impl>(path);
}

#include "fetch_request.h"

#include <string>

#include <cstring>

#include <emscripten/fetch.h>

namespace {

class fetch_request_impl final : public fetch_request
{
public:
  explicit fetch_request_impl(const char* path)
    : m_path(path)
  {
    emscripten_fetch_attr_init(&m_fetch_attr);

    std::strcpy(m_fetch_attr.requestMethod, "GET");
    m_fetch_attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    m_fetch_attr.onsuccess = on_success;
    m_fetch_attr.onerror = on_failure;
    m_fetch_attr.userData = this;
    emscripten_fetch(&m_fetch_attr, path);
  }

  void iterate() override {}

  auto done() const -> bool override { return m_done; }

  auto success() const -> bool override { return m_success; }

  auto get_data() const -> const void* override { return m_data.data(); }

  auto get_size() const -> std::size_t override { return m_data.size(); }

protected:
  static void on_success(emscripten_fetch_t* fetch)
  {
    auto* self = static_cast<fetch_request_impl*>(fetch->userData);

    self->m_data = std::string(fetch->data, fetch->numBytes);

    self->m_done = true;
    self->m_success = true;

    emscripten_fetch_close(fetch);
  }

  static void on_failure(emscripten_fetch_t* fetch)
  {
    auto* self = static_cast<fetch_request_impl*>(fetch->userData);

    self->m_done = true;

    emscripten_fetch_close(fetch);
  }

private:
  std::string m_path;

  bool m_done{ false };

  bool m_success{ false };

  emscripten_fetch_attr_t m_fetch_attr{};

  std::string m_data;
};

} // namespace

auto
fetch_request::create(const char* path) -> std::unique_ptr<fetch_request>
{
  return std::unique_ptr<fetch_request>(new fetch_request_impl(path));
}

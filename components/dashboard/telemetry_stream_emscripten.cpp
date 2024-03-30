#include "telemetry_stream.h"

#include <emscripten/fetch.h>

#include <sstream>

namespace {

class telemetry_stream_impl final : public telemetry_stream
{
public:
  telemetry_stream_impl(const std::string& host, int port)
    : m_host(host)
    , m_port(port)
  {
    {
      std::ostringstream path_stream;
      path_stream << "http://" << host << ':' << port << '/';
      m_path = path_stream.str();
    }

    emscripten_fetch_attr_init(&m_fetch_attr);
    m_fetch_attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    m_fetch_attr.onsuccess = on_success;
    m_fetch_attr.onerror = on_failure;
    m_fetch_attr.userData = this;
  }

  void request_new_telemetry() override
  {
    emscripten_fetch(&m_fetch_attr, m_path.c_str());
    m_has_new = false;
  }

  auto check_new_telemetry() -> bool override
  {
    const auto ret = m_has_new;
    m_has_new = false;
    return ret;
  }

  void add_observer(observer* o) override { m_observers.emplace_back(o); }

protected:
  static void on_success(emscripten_fetch_t* fetch)
  {
    auto* self = static_cast<telemetry_stream_impl*>(fetch->userData);

    self->m_has_new = true;

    for (auto* o : self->m_observers) {
      o->observe(fetch->data, fetch->numBytes);
    }

    emscripten_fetch_close(fetch);
  }

  static void on_failure(emscripten_fetch_t* fetch) {}

private:
  std::string m_host;

  int m_port{ 0 };

  std::vector<observer*> m_observers;

  emscripten_fetch_attr_t m_fetch_attr{};

  std::string m_path;

  bool m_has_new{ false };
};

} // namespace

auto
telemetry_stream::create(const char* host, const int port, const config::sensor_type type)
  -> std::unique_ptr<telemetry_stream>
{
  return std::unique_ptr<telemetry_stream>(new telemetry_stream_impl(host, port));
}

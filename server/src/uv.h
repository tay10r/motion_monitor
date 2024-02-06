#pragma once

#include <spdlog/spdlog.h>

#include <vector>

#include <uv.h>

#include <cstdint>

inline auto
to_handle(uv_tcp_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

inline auto
to_handle(uv_stream_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

inline auto
to_handle(uv_write_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

inline auto
to_handle(uv_timer_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

class write_operation final
{
public:
  using complete_cb = void (*)(void*, bool success);

  static void send(uv_stream_t* socket, std::vector<std::uint8_t> data, void* cb_data, complete_cb cb_func)
  {
    auto* op = new write_operation(std::move(data), cb_data, cb_func);

    if (!op->send(socket)) {
      spdlog::error("Failed to send message.");
      if (cb_func) {
        cb_func(cb_data, false);
      }
      delete op;
    }
  }

protected:
  write_operation(std::vector<std::uint8_t> data, void* cb_data, complete_cb cb_func)
    : m_data(std::move(data))
    , m_cb_data(cb_data)
    , m_cb_func(cb_func)
  {
    m_buffer.base = reinterpret_cast<char*>(&m_data[0]);
    m_buffer.len = m_data.size();

    uv_handle_set_data(to_handle(&m_handle), this);
  }

  auto send(uv_stream_t* socket) -> bool { return uv_write(&m_handle, socket, &m_buffer, 1, on_write_complete) == 0; }

  static void on_write_complete(uv_write_t* handle, const int status)
  {
    auto* self = static_cast<write_operation*>(uv_handle_get_data(to_handle(handle)));

    if (self->m_cb_func) {
      self->m_cb_func(self->m_cb_data, status == 0);
    }

    delete self;

    if (status != 0) {
      spdlog::error("Failed to complete write operation ({}).", uv_strerror(status));
    }
  }

private:
  std::vector<std::uint8_t> m_data;

  uv_buf_t m_buffer{};

  uv_write_t m_handle{};

  void* m_cb_data{ nullptr };

  complete_cb m_cb_func{ nullptr };
};

#include "pipeline_runner.h"

namespace {

auto
to_handle(uv_async_t* handle) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(handle);
}

} // namespace

pipeline_runner::pipeline_runner(uv_loop_t* loop, std::unique_ptr<pipeline> p)
  : m_thread(&pipeline_runner::run_pipeline, this)
  , m_pipeline(std::move(p))
{
  uv_async_init(loop, &m_handle, on_async_update);

  uv_handle_set_data(to_handle(&m_handle), this);
}

void
pipeline_runner::close()
{
  m_should_close.store(true);

  if (m_thread.joinable()) {
    m_thread.join();
  }

  uv_close(to_handle(&m_handle), nullptr);
}

auto
pipeline_runner::get_self(uv_handle_t* handle) -> pipeline_runner*
{
  return static_cast<pipeline_runner*>(uv_handle_get_data(handle));
}

void
pipeline_runner::run_pipeline()
{
  while (!m_should_close.load()) {

    auto should_close = false;

    auto outs = m_pipeline->loop(should_close);

    {
      std::lock_guard<std::mutex> lock(m_lock);

      for (auto& out : outs) {
        m_outputs.emplace_back(std::move(out));
      }
    }

    uv_async_send(&m_handle);

    if (should_close) {
      break;
    }
  }
}

void
pipeline_runner::on_async_update(uv_async_t* handle)
{
  auto* self = get_self(to_handle(handle));

  std::vector<std::shared_ptr<sentinel::proto::outbound_message>> outs;

  {
    std::lock_guard<std::mutex> lock(self->m_lock);

    outs = std::move(self->m_outputs);
  }

  for (auto& out : outs) {
    for (auto* obs : self->m_observers) {
      obs->observe_telemetry(out);
    }
  }
}

void
pipeline_runner::add_telemetry_observer(telemetry_observer* o)
{
  m_observers.emplace_back(o);
}

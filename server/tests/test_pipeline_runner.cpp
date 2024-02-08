#include <gtest/gtest.h>

#include "../src/pipeline_runner.h"

#include <chrono>
#include <thread>

#include <iostream>

namespace {

class fake_pipeline final : public pipeline
{
public:
  auto loop(bool& should_close) -> std::vector<std::vector<std::uint8_t>> override
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    std::vector<std::vector<std::uint8_t>> output;

    m_counter++;

    if (m_counter == 3) {

      output.emplace_back(std::vector<std::uint8_t>{});

      should_close = true;
    }

    return output;
  }

  auto count() const -> int { return m_counter; }

private:
  int m_counter{ 0 };
};

class observer final : public telemetry_observer
{
public:
  explicit observer(uv_loop_t* loop)
    : m_loop(loop)
  {
  }

  void observe_telemetry(const std::uint8_t*, const std::size_t) override { uv_stop(m_loop); }

private:
  uv_loop_t* m_loop{ nullptr };
};

} // namespace

TEST(PipelineRunner, StartStop)
{
  uv_loop_t loop{};

  uv_loop_init(&loop);

  pipeline_runner runner(&loop, std::make_unique<fake_pipeline>());

  observer obs(&loop);

  runner.add_telemetry_observer(&obs);

  uv_run(&loop, UV_RUN_DEFAULT);

  runner.close();

  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);
}

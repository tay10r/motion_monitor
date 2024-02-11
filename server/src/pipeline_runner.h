#pragma once

#include <uv.h>

#include <atomic>
#include <mutex>
#include <thread>

#include "pipeline.h"
#include "telemetry_observer.h"

/**
 * @brief Used for running a pipeline in asynchronously.
 * */
class pipeline_runner final
{
public:
  /**
   * @brief Constructs a new pipeline runner.
   *
   * @param loop The loop to construct the thread interface to.
   *
   * @param p The pipeline to run asynchronously.
   * */
  explicit pipeline_runner(uv_loop_t* loop, std::unique_ptr<pipeline> p);

  /**
   * @brief Closes the runner and the pipeline.
   * */
  void close();

  /**
   * @brief Adds an observer for when the pipeline produces telemetry.
   *
   * @param o The observer to add to the pipeline runner.
   * */
  void add_telemetry_observer(telemetry_observer* o);

protected:
  static auto get_self(uv_handle_t* handle) -> pipeline_runner*;

  /**
   * @brief Called when the pipeline produces output.
   * */
  static void on_async_update(uv_async_t* handle);

  /**
   * @note This is the entrypoint of the worker thread.
   * */
  void run_pipeline();

private:
  /**
   * @brief The thread to run the pipeline on.
   * */
  std::thread m_thread;

  /**
   * @brief The pipeline to run asynchronously.
   * */
  std::unique_ptr<pipeline> m_pipeline;

  /**
   * @brief The lock for the telemetry output.
   * */
  std::mutex m_lock;

  /**
   * @brief The pipeline telemetry output.
   * */
  std::vector<std::shared_ptr<sentinel::proto::outbound_message>> m_outputs;

  /**
   * @brief The observers to listen to telemetry with.
   * */
  std::vector<telemetry_observer*> m_observers;

  /**
   * @brief Indicates whether or not the thread should close.
   * */
  std::atomic<bool> m_should_close{ false };

  uv_async_t m_handle{};
};

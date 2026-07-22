/**
 * Software Rasterizer Playground.
 *
 * Background task system.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <concurrency_utils/thread_pool.h>

#include "containers/vector.h"
#include "task_logger.h"
#include "state.h"

namespace task_system
{

/** Exception thrown when task execution is cancelled. */
class TaskCancelledError final
: public std::runtime_error
{
public:
    /** Construct a cancellation exception. */
    TaskCancelledError();
};

/** Provides progress reporting and cancellation checks to running tasks. */
class TaskExecutionContext
{
    /** Shared state (e.g. cancellation and progress updates). */
    std::shared_ptr<struct TaskSharedState> state;

    /** Base progress offset within the enclosing task range. */
    float progress_base{0.f};

    /** Progress scale within the enclosing task range. */
    float progress_scale{1.f};

    /** Optional task index targeted by context progress updates. */
    std::optional<std::size_t> aggregate_task_index;

public:
    /**
     * Creates a task execution context.
     *
     * @param state Shared task state backing progress/cancellation.
     * @param progress_base Base progress offset in [0, 1].
     * @param progress_scale Progress scaling factor for local updates.
     * @param aggregate_task_index Optional task slot index updated by this context.
     */
    TaskExecutionContext(
      std::shared_ptr<TaskSharedState> state,
      float progress_base,
      float progress_scale,
      std::optional<std::size_t> aggregate_task_index = std::nullopt);

    /**
     * Updates task status text and normalized progress.
     *
     * @param status_text New status text to publish.
     * @param progress Local progress value, clamped to [0, 1].
     */
    void update(
      std::string status_text,
      float progress) const;

    /**
     * Returns true if cancellation has been requested.
     *
     * @returns True when cancellation was requested, otherwise false.
     */
    [[nodiscard]]
    bool is_cancel_requested() const noexcept;

    /**
     * Creates a child context mapped to a subrange of this context.
     *
     * @param start Subrange start in parent-local normalized coordinates.
     * @param scale Subrange scale factor relative to the parent range.
     * @returns Child execution context scoped to the requested subrange.
     */
    [[nodiscard]]
    TaskExecutionContext subrange(
      float start,
      float scale) const;
};

/** Callable task unit receiving a cancellation-aware execution context. */
using Task = std::function<void(TaskExecutionContext&)>;

/** Describes a task, its dependencies, and scheduling metadata. */
struct TaskSpec
{
    /** Optional task name used for progress status reporting. */
    std::string name{};

    /** Relative task weight used for weighted aggregate progress. */
    float weight{1.f};

    /** Indices of prerequisite tasks. */
    swr::vector<std::size_t> dependencies{};

    /** Optional predicate deciding whether the task should run. */
    std::function<bool()> start_condition{};

    /** Task body to execute when scheduled. */
    Task run{};
};

/**
 * Runs tasks in a provided dependency-valid execution order.
 *
 * @param context Execution context used for progress/cancellation.
 * @param tasks Task list to execute.
 * @param execution_order Dependency-valid task index order.
 */
void run_task_specs(
  TaskExecutionContext& context,
  const swr::vector<TaskSpec>& tasks,
  const swr::vector<std::size_t>& execution_order);

/** Lightweight handle for controlling and observing submitted work. */
class TaskHandle
{
    /** Shared state of the associated submitted task group. */
    std::shared_ptr<TaskSharedState> state;

public:
    /** Constructs an empty, invalid task handle. */
    TaskHandle() = default;

    /**
     * Constructs a handle bound to shared task state.
     *
     * @param state Shared task state to reference.
     */
    explicit TaskHandle(
      std::shared_ptr<TaskSharedState> state);

    /**
     * Returns true if this handle references a task state.
     *
     * @returns True when the handle references valid shared state.
     */
    [[nodiscard]]
    bool valid() const noexcept;

    /** Requests cooperative cancellation for the associated work. */
    void cancel() const noexcept;

    /**
     * Returns true if execution has finished.
     *
     * @returns True when associated work reached its terminal state.
     */
    [[nodiscard]]
    bool is_finished() const noexcept;

    /**
     * Returns the latest submission snapshot.
     *
     * @returns Latest aggregate and per-task snapshot data.
     */
    [[nodiscard]]
    TaskGroupSnapshot snapshot() const;

    /** Blocks until execution is marked finished. */
    void wait() const;
};

/** Bundles a task handle and result future for a submission. */
template<typename Result>
struct TaskSubmission
{
    /** Handle for cancellation, status polling, and waiting. */
    TaskHandle handle;

    /** Future that resolves with task result or exception. */
    std::future<Result> future;
};

/** Schedules background tasks on a deferred thread pool. */
class TaskSystem
{
    /** Logger used for task-system lifecycle messages. */
    const TaskLogger& logger;

    /** Worker pool used to run submitted tasks. */
    concurrency_utils::deferred_thread_pool<> thread_pool;

public:
    /**
     * Creates a task system with a fixed number of worker threads.
     *
     * @param worker_count Number of worker threads in the pool.
     * @param task_logger Logger for task-system messages. Defaults to a null logger.
     */
    explicit TaskSystem(
      std::size_t worker_count,
      const TaskLogger& task_logger = NullTaskLogger::instance());

    /**
     * Submits dependency-aware task specs and returns handle/future pair.
     * Multiple submissions execute concurrently; use future.get() to enforce ordering.
     *
     * @param tasks Task specifications to validate and schedule.
     * @returns Submission object containing a handle and completion future.
     */
    TaskSubmission<void> submit_task_specs(
      swr::vector<TaskSpec> tasks);

    /**
     * Submits a callable that receives a TaskExecutionContext.
     *
     * @param fn Callable invoked with a TaskExecutionContext reference.
     * @returns Submission object exposing task control and result future.
     */
    template<typename Fn>
    auto submit(Fn&& fn)
      -> TaskSubmission<
        std::invoke_result_t<
          std::decay_t<Fn>,
          TaskExecutionContext&>>
    {
        using Function = std::decay_t<Fn>;
        using Result = std::invoke_result_t<Function, TaskExecutionContext&>;

        auto state = std::make_shared<TaskSharedState>();
        {
            std::scoped_lock lock{state->snapshot_mutex};
            state->snapshot.progress = 0.f;
            state->snapshot.tasks = {
              TaskSnapshot{
                .name = {},
                .status_text = "Queued...",
              },
            };
            state->task_weights = {1.f};
            state->total_weight = 1.f;
        }

        auto promise = std::make_shared<std::promise<Result>>();
        auto future = promise->get_future();

        thread_pool.push_immediate_task(
          [state, promise, fn = Function{std::forward<Fn>(fn)}]() mutable
          {
              auto mark_running = [&state]()
              {
                  std::scoped_lock lock{state->snapshot_mutex};
                  TaskSnapshot& snapshot = state->snapshot.tasks.front();
                  snapshot.state = TaskState::Running;
              };

              auto mark_terminal = [&state](
                                     TaskState task_state,
                                     const char* default_status_text,
                                     std::optional<float> task_progress = std::nullopt)
              {
                  std::scoped_lock lock{state->snapshot_mutex};
                  TaskSnapshot& snapshot = state->snapshot.tasks.front();
                  snapshot.state = task_state;
                  if(task_progress.has_value())
                  {
                      snapshot.progress = std::clamp(*task_progress, 0.f, 1.f);
                  }
                  if(snapshot.status_text.empty())
                  {
                      snapshot.status_text = default_status_text;
                  }
                  if(task_state == TaskState::Completed)
                  {
                      state->snapshot.progress = 1.f;
                  }
              };

              TaskExecutionContext context{
                state,
                0.f,
                1.f,
                0,
              };

              try
              {
                  mark_running();

                  if constexpr(std::is_void_v<Result>)
                  {
                      fn(context);
                      mark_terminal(
                        TaskState::Completed,
                        "Done.",
                        1.f);
                      promise->set_value();
                  }
                  else
                  {
                      Result result = fn(context);
                      mark_terminal(
                        TaskState::Completed,
                        "Done.",
                        1.f);
                      promise->set_value(std::move(result));
                  }
              }
              catch(const TaskCancelledError&)
              {
                  mark_terminal(
                    TaskState::Cancelled,
                    "Cancelled.");
                  promise->set_exception(std::current_exception());
              }
              catch(...)
              {
                  mark_terminal(
                    TaskState::Failed,
                    "Failed.");
                  promise->set_exception(std::current_exception());
              }

              state->finished.store(true, std::memory_order_relaxed);
              {
                  std::lock_guard lock{state->finished_mutex};
              }
              state->finished_cv.notify_all();
          });

        return TaskSubmission<Result>{
          .handle = TaskHandle{state},
          .future = std::move(future),
        };
    }
};

}    // namespace task_system

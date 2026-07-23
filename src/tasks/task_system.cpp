/**
 * Software Rasterizer Playground.
 *
 * Background task system.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cstddef>
#include <format>
#include <stdexcept>

#include "containers/format.h"
#include "task_system.h"
#include "dag.h"
#include "scheduler.h"

namespace task_system
{

// Helper function used by TaskExecutionContext.
std::optional<std::size_t> resolve_task_index_locked(
  const TaskSharedState& state,
  const std::optional<std::size_t>& task_index)
{
    if(task_index.has_value())
    {
        if(task_index.value() < state.snapshot.tasks.size())
        {
            return task_index;
        }

        return std::nullopt;
    }

    if(state.snapshot.tasks.size() == 1)
    {
        return std::make_optional<std::size_t>(0uz);
    }

    return std::nullopt;
}

void initialize_task_group_snapshot(
  TaskSharedState& state,
  const swr::vector<TaskSpec>& tasks,
  const swr::vector<float>& weights)
{
    std::scoped_lock lock{state.snapshot_mutex};
    state.snapshot.tasks.clear();
    state.snapshot.tasks.reserve(tasks.size());
    for(const TaskSpec& task: tasks)
    {
        state.snapshot.tasks.push_back(TaskSnapshot{
          .name = task.name,
          .status_text = "Queued...",
          .state = TaskState::Queued});
    }
    state.task_weights = weights;
    state.total_weight = 0.f;
    for(const float weight: weights)
    {
        state.total_weight += weight;
    }
    state.snapshot.progress = 0.f;
}

void run_task_specs(
  TaskExecutionContext& context,
  const swr::vector<TaskSpec>& tasks,
  const swr::vector<std::size_t>& execution_order)
{
    if(tasks.empty())
    {
        context.update("No tasks.", 1.f);
        return;
    }

    if(execution_order.size() != tasks.size())
    {
        throw std::runtime_error{"Task execution order size mismatch"};
    }

    swr::vector<float> normalized_weights;
    normalized_weights.reserve(tasks.size());
    float total_weight = 0.f;
    for(const TaskSpec& task: tasks)
    {
        const float weight = std::max(task.weight, 1.f);
        normalized_weights.push_back(weight);
        total_weight += weight;
    }

    float completed_weight = 0.f;
    for(const std::size_t task_index: execution_order)
    {
        if(task_index >= tasks.size())
        {
            throw std::out_of_range{"Task execution order index out of range"};
        }

        if(context.is_cancel_requested())
        {
            throw TaskCancelledError{};
        }

        const TaskSpec& task = tasks[task_index];
        const float task_weight = normalized_weights[task_index];
        const float task_start = completed_weight / total_weight;
        const float task_span = task_weight / total_weight;
        TaskExecutionContext task_context = context.subrange(
          task_start,
          task_span);

        const bool should_run =
          !task.start_condition || task.start_condition();
        if(!should_run)
        {
            const swr::string skipped_status = task.name.empty()
                                                 ? "Skipped task"
                                                 : swr::format(
                                                     "{} (skipped)",
                                                     task.name);
            task_context.update(skipped_status, 1.f);
            completed_weight += task_weight;
            continue;
        }

        if(!task.name.empty())
        {
            task_context.update(task.name, 0.f);
        }

        if(task.run)
        {
            task.run(task_context);
        }

        if(context.is_cancel_requested())
        {
            throw TaskCancelledError{};
        }

        if(!task.name.empty())
        {
            task_context.update(task.name, 1.f);
        }

        completed_weight += task_weight;
    }

    context.update("Done.", 1.f);
}

/*
 * TaskCancelledError.
 */

TaskCancelledError::TaskCancelledError()
: std::runtime_error{"Task cancelled"}
{
}

/*
 * TaskExecutionContext.
 */

TaskExecutionContext::TaskExecutionContext(
  std::shared_ptr<TaskSharedState> state,
  float progress_base,
  float progress_scale,
  std::optional<std::size_t> aggregate_task_index)
: state{std::move(state)}
, progress_base{progress_base}
, progress_scale{progress_scale}
, aggregate_task_index{aggregate_task_index}
{
}

void TaskExecutionContext::update(
  std::string_view status_text,
  float progress) const
{
    if(!state)
    {
        return;
    }

    const float local_progress = std::clamp(progress, 0.f, 1.f);

    const float overall_progress = std::clamp(
      progress_base + progress_scale * local_progress,
      0.f,
      1.f);

    {
        std::scoped_lock lock{state->snapshot_mutex};
        if(const auto task_index = resolve_task_index_locked(
             *state,
             aggregate_task_index);
           task_index.has_value())
        {
            TaskSnapshot& task_snapshot = state->snapshot.tasks[task_index.value()];
            if(!status_text.empty())
            {
                task_snapshot.status_text = std::move(status_text);
            }

            task_snapshot.progress = std::max(task_snapshot.progress, local_progress);

            if(task_snapshot.state == TaskState::Queued
               || task_snapshot.state == TaskState::Running)
            {
                task_snapshot.state = task_snapshot.progress >= 1.f
                                        ? TaskState::Completed
                                        : TaskState::Running;
            }

            detail::refresh_group_progress_locked(*state);
            return;
        }

        state->snapshot.progress = std::max(state->snapshot.progress, overall_progress);
    }
}

bool TaskExecutionContext::is_cancel_requested() const noexcept
{
    return state
           && state->cancel_requested.load(std::memory_order_relaxed);
}

TaskExecutionContext TaskExecutionContext::subrange(
  float start,
  float scale) const
{
    return TaskExecutionContext{
      state,
      progress_base + progress_scale * std::clamp(start, 0.f, 1.f),
      progress_scale * std::max(0.f, scale),
      aggregate_task_index,
    };
}

/*
 * TaskHandle.
 */

TaskHandle::TaskHandle(
  std::shared_ptr<TaskSharedState> state)
: state{std::move(state)}
{
}

bool TaskHandle::valid() const noexcept
{
    return static_cast<bool>(state);
}

void TaskHandle::cancel() const noexcept
{
    if(state)
    {
        state->cancel_requested.store(true, std::memory_order_relaxed);
    }
}

bool TaskHandle::is_finished() const noexcept
{
    return state
           && state->finished.load(std::memory_order_relaxed);
}

TaskGroupSnapshot TaskHandle::snapshot() const
{
    if(!state)
    {
        return TaskGroupSnapshot{};
    }

    std::scoped_lock lock{state->snapshot_mutex};
    return state->snapshot;
}

void TaskHandle::wait() const
{
    if(!state)
    {
        return;
    }

    std::unique_lock lock{state->finished_mutex};
    state->finished_cv.wait(
      lock,
      [state = state]
      {
          return state->finished.load(std::memory_order_relaxed);
      });
}

/*
 * TaskSystem.
 */

TaskSystem::TaskSystem(
  std::size_t worker_count,
  const TaskLogger& task_logger)
: logger{task_logger}
, thread_pool{worker_count}
{
    logger.logf(
      "Using {} worker threads.",
      worker_count);
}

TaskSubmission<void> TaskSystem::submit_task_specs(
  swr::vector<TaskSpec> tasks)
{
    // Validate and freeze deterministic topological order at submission time.
    const swr::vector<std::size_t> execution_order =
      build_task_execution_order(tasks);

    TaskSchedulingData scheduling_data = build_task_scheduling_data(
      tasks,
      execution_order);

    auto state = std::make_shared<TaskSharedState>();
    initialize_task_group_snapshot(*state, tasks, scheduling_data.weights);

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    thread_pool.push_immediate_task(
      [this,
       state,
       promise,
       tasks = std::move(tasks),
       scheduling_data = std::move(scheduling_data)]() mutable
      {
          run_task_specs_scheduler(
            thread_pool,
            state,
            promise,
            std::move(tasks),
            std::move(scheduling_data));
      });

    return TaskSubmission<void>{
      .handle = TaskHandle{state},
      .future = std::move(future),
    };
}

}    // namespace task_system

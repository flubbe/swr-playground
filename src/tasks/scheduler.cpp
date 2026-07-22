/**
 * Software Rasterizer Playground.
 *
 * Parallel task graph scheduler.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "scheduler.h"

#include <algorithm>
#include <exception>
#include <format>
#include <stdexcept>

namespace task_system
{

namespace
{

struct SchedulerShared
{
    std::mutex mutex;
    std::condition_variable cv;
    swr::vector<std::size_t> finished_tasks;
    std::size_t running_tasks{0};
    std::exception_ptr first_error;
};

struct SchedulerProgress
{
    swr::vector<std::size_t> finished_tasks;
    std::size_t running_tasks{0};
    std::exception_ptr error;
};

/**
 * Return a sorted list (descending) of indices with indegree `0`.
 *
 * @param indegree Indegrees.
 * @returns Returns the `ready` indices into `indegree`.
 */
[[nodiscard]]
swr::vector<std::size_t> build_sorted_ready_queue(
  const swr::vector<std::size_t>& indegree)
{
    swr::vector<std::size_t> ready;
    ready.reserve(indegree.size());

    for(std::size_t i = 0; i < indegree.size(); ++i)
    {
        if(indegree[i] == 0)
        {
            ready.push_back(i);
        }
    }

    std::ranges::reverse(ready);
    return ready;
}

void run_scheduled_task_worker(
  const std::size_t task_index,
  TaskSpec task,
  const float progress_base,
  const float progress_span,
  const std::shared_ptr<TaskSharedState>& task_state,
  const std::shared_ptr<SchedulerShared>& shared)
{
    TaskExecutionContext task_context{
      task_state,
      progress_base,
      progress_span,
      task_index,
    };

    auto fail_task_and_cancel =
      [&](TaskState task_state_value, std::string status_text)
    {
        detail::set_task_state(
          *task_state,
          task_index,
          task_state_value,
          std::move(status_text));

        std::scoped_lock lock{shared->mutex};
        if(!shared->first_error)
        {
            shared->first_error = std::current_exception();
        }
        task_state->cancel_requested.store(true, std::memory_order_relaxed);
    };

    try
    {
        if(task_context.is_cancel_requested())
        {
            throw TaskCancelledError{};
        }

        if(!task.name.empty())
        {
            task_context.update(task.name, 0.f);
        }

        if(task.run)
        {
            task.run(task_context);
        }

        if(task_context.is_cancel_requested())
        {
            throw TaskCancelledError{};
        }

        if(!task.name.empty())
        {
            task_context.update(task.name, 1.f);
        }

        detail::set_task_state(
          *task_state,
          task_index,
          TaskState::Completed,
          task.name.empty()
            ? std::make_optional<std::string>("Done.")
            : std::make_optional<std::string>(task.name),
          1.f);
    }
    catch(const TaskCancelledError&)
    {
        fail_task_and_cancel(
          TaskState::Cancelled,
          "Cancelled.");
    }
    catch(...)
    {
        fail_task_and_cancel(
          TaskState::Failed,
          "Failed.");
    }

    {
        std::scoped_lock lock{shared->mutex};
        shared->finished_tasks.push_back(task_index);
        if(shared->running_tasks > 0)
        {
            --shared->running_tasks;
        }
    }
    shared->cv.notify_one();
}

void sort_ready_tasks(
  swr::vector<std::size_t>& ready,
  const swr::vector<std::size_t>& execution_rank)
{
    std::ranges::sort(
      ready,
      [&](const std::size_t lhs, const std::size_t rhs)
      {
          return execution_rank[lhs] > execution_rank[rhs];
      });
}

void mark_task_completed_and_release_dependents(
  const std::size_t finished_task_index,
  std::size_t& completed_tasks,
  swr::vector<std::size_t>& ready,
  swr::vector<std::size_t>& indegree,
  const swr::vector<swr::vector<std::size_t>>& dependents)
{
    ++completed_tasks;
    for(const std::size_t dependent_index: dependents[finished_task_index])
    {
        if(indegree[dependent_index] > 0)
        {
            --indegree[dependent_index];
            if(indegree[dependent_index] == 0)
            {
                ready.push_back(dependent_index);
            }
        }
    }
}

void dispatch_ready_tasks(
  concurrency_utils::deferred_thread_pool<>& thread_pool,
  const std::shared_ptr<TaskSharedState>& state,
  const std::shared_ptr<SchedulerShared>& scheduler_shared,
  const swr::vector<TaskSpec>& tasks,
  const swr::vector<float>& task_progress_base,
  const swr::vector<float>& task_progress_span,
  swr::vector<std::size_t>& ready,
  swr::vector<std::size_t>& indegree,
  const swr::vector<swr::vector<std::size_t>>& dependents,
  const swr::vector<std::size_t>& execution_rank,
  std::size_t& completed_tasks)
{
    while(!ready.empty()
          && !state->cancel_requested.load(std::memory_order_relaxed))
    {
        {
            std::scoped_lock lock{scheduler_shared->mutex};
            if(scheduler_shared->first_error)
            {
                break;
            }
        }

        const std::size_t task_index = ready.back();
        ready.pop_back();

        const TaskSpec& task = tasks[task_index];
        const bool should_run =
          !task.start_condition || task.start_condition();

        if(!should_run)
        {
            const std::string skipped_status = task.name.empty()
                                                 ? "Skipped task"
                                                 : std::format(
                                                     "{} (skipped)",
                                                     task.name);
            detail::set_task_state(
              *state,
              task_index,
              TaskState::Skipped,
              skipped_status,
              1.f);

            mark_task_completed_and_release_dependents(
              task_index,
              completed_tasks,
              ready,
              indegree,
              dependents);
            sort_ready_tasks(ready, execution_rank);
            continue;
        }

        {
            std::scoped_lock lock{scheduler_shared->mutex};
            ++scheduler_shared->running_tasks;
        }

        detail::set_task_state(
          *state,
          task_index,
          TaskState::Running,
          task.name.empty() ? std::nullopt : std::make_optional<std::string>(task.name),
          0.f);

        TaskSpec task_copy = task;
        const float progress_base = task_progress_base[task_index];
        const float progress_span = task_progress_span[task_index];

        thread_pool.push_immediate_task(
          [task_index,
           task_copy = std::move(task_copy),
           progress_base,
           progress_span,
           task_state = state,
           shared = scheduler_shared]() mutable
          {
              run_scheduled_task_worker(
                task_index,
                std::move(task_copy),
                progress_base,
                progress_span,
                task_state,
                shared);
          });
    }
}

[[nodiscard]]
SchedulerProgress wait_for_scheduler_progress(
  const std::shared_ptr<SchedulerShared>& scheduler_shared)
{
    SchedulerProgress progress;
    {
        std::unique_lock lock{scheduler_shared->mutex};
        scheduler_shared->cv.wait(
          lock,
          [&]()
          {
              return !scheduler_shared->finished_tasks.empty()
                     || scheduler_shared->running_tasks == 0;
          });

        progress.finished_tasks.swap(scheduler_shared->finished_tasks);
        progress.running_tasks = scheduler_shared->running_tasks;
        progress.error = scheduler_shared->first_error;
    }

    return progress;
}

void wait_for_scheduler_idle_or_throw(
  const std::shared_ptr<SchedulerShared>& scheduler_shared)
{
    std::unique_lock lock{scheduler_shared->mutex};
    scheduler_shared->cv.wait(
      lock,
      [&]()
      {
          return scheduler_shared->running_tasks == 0;
      });

    if(scheduler_shared->first_error)
    {
        std::rethrow_exception(scheduler_shared->first_error);
    }
}

}    // namespace

void run_task_specs_scheduler(
  concurrency_utils::deferred_thread_pool<>& thread_pool,
  const std::shared_ptr<TaskSharedState>& state,
  const std::shared_ptr<std::promise<void>>& promise,
  swr::vector<TaskSpec> tasks,
  TaskSchedulingData scheduling_data)
{
    auto scheduler_shared = std::make_shared<SchedulerShared>();

    try
    {
        swr::vector<std::size_t>& indegree = scheduling_data.indegree;
        swr::vector<swr::vector<std::size_t>>& dependents = scheduling_data.dependents;
        swr::vector<std::size_t>& execution_rank = scheduling_data.execution_rank;
        swr::vector<float>& task_progress_base = scheduling_data.task_progress_base;
        swr::vector<float>& task_progress_span = scheduling_data.task_progress_span;

        swr::vector<std::size_t> ready = build_sorted_ready_queue(indegree);
        sort_ready_tasks(ready, execution_rank);
        std::size_t completed_tasks = 0;

        while(completed_tasks < tasks.size())
        {
            if(state->cancel_requested.load(std::memory_order_relaxed))
            {
                throw TaskCancelledError{};
            }

            dispatch_ready_tasks(
              thread_pool,
              state,
              scheduler_shared,
              tasks,
              task_progress_base,
              task_progress_span,
              ready,
              indegree,
              dependents,
              execution_rank,
              completed_tasks);

            if(completed_tasks >= tasks.size())
            {
                break;
            }

            const SchedulerProgress progress = wait_for_scheduler_progress(
              scheduler_shared);

            if(progress.error)
            {
                std::rethrow_exception(progress.error);
            }

            for(const std::size_t finished_task_index: progress.finished_tasks)
            {
                mark_task_completed_and_release_dependents(
                  finished_task_index,
                  completed_tasks,
                  ready,
                  indegree,
                  dependents);
            }
            sort_ready_tasks(ready, execution_rank);

            const bool no_ready_tasks = ready.empty();
            const bool no_running_tasks = progress.running_tasks == 0;
            const bool work_remaining = completed_tasks < tasks.size();
            if(no_ready_tasks && no_running_tasks && work_remaining)
            {
                throw std::runtime_error{
                  "Task dependency graph could not be resolved (cycle or unsatisfied dependencies)"};
            }
        }

        wait_for_scheduler_idle_or_throw(scheduler_shared);

        if(state->cancel_requested.load(std::memory_order_relaxed))
        {
            throw TaskCancelledError{};
        }

        promise->set_value();
    }
    catch(const TaskCancelledError&)
    {
        detail::cancel_unfinished_tasks(*state);
        promise->set_exception(std::current_exception());
    }
    catch(...)
    {
        if(state->cancel_requested.load(std::memory_order_relaxed))
        {
            detail::cancel_unfinished_tasks(*state);
        }
        promise->set_exception(std::current_exception());
    }

    state->finished.store(true, std::memory_order_relaxed);
    state->finished_cv.notify_all();
}

}    // namespace task_system

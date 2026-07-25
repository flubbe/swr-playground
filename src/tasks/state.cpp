/**
 * Software Rasterizer Playground.
 *
 * Task system internal state types and manipulation (implementation).
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>

#include "state.h"

namespace task_system::detail
{

namespace
{

/**
 * Compute the total progress of a task group.
 *
 * @note The snapshot mutex must be held by the caller.
 *
 * @param state Shared task state.
 * @returns Task group progress.
 */
[[nodiscard]]
float compute_group_progress_locked(
  const TaskSharedState& state)
{
    if(state.snapshot.tasks.empty())
    {
        return 0.f;
    }

    if(state.task_weights.size() != state.snapshot.tasks.size()
       || state.total_weight <= 0.f)
    {
        float total_progress = 0.f;
        for(const TaskSnapshot& task: state.snapshot.tasks)
        {
            total_progress += std::clamp(task.progress, 0.f, 1.f);
        }
        return total_progress / static_cast<float>(state.snapshot.tasks.size());
    }

    float weighted_progress = 0.f;
    for(std::size_t i = 0; i < state.snapshot.tasks.size(); ++i)
    {
        weighted_progress += std::clamp(state.snapshot.tasks[i].progress, 0.f, 1.f)
                             * state.task_weights[i];
    }

    return weighted_progress / state.total_weight;
}

}    // namespace

void refresh_group_progress_locked(TaskSharedState& state)
{
    state.snapshot.progress = std::clamp(
      compute_group_progress_locked(state),
      0.f,
      1.f);
}

void set_task_state(
  TaskSharedState& state,
  std::size_t task_index,
  TaskState task_state,
  std::optional<swr::string> status_text,
  std::optional<float> progress)
{
    std::scoped_lock lock{state.snapshot_mutex};
    if(task_index >= state.snapshot.tasks.size())
    {
        return;
    }

    TaskSnapshot& task_snapshot = state.snapshot.tasks[task_index];
    if(status_text.has_value())
    {
        task_snapshot.status_text = std::move(*status_text);
    }
    if(progress.has_value())
    {
        task_snapshot.progress = std::clamp(*progress, 0.f, 1.f);
    }

    task_snapshot.state = task_state;
    refresh_group_progress_locked(state);
}

void cancel_unfinished_tasks(TaskSharedState& state)
{
    std::scoped_lock lock{state.snapshot_mutex};
    for(TaskSnapshot& task_snapshot: state.snapshot.tasks)
    {
        if(task_snapshot.state == TaskState::Queued
           || task_snapshot.state == TaskState::Running)
        {
            task_snapshot.state = TaskState::Cancelled;
            if(task_snapshot.status_text.empty())
            {
                task_snapshot.status_text = "Cancelled.";
            }
        }
    }
    refresh_group_progress_locked(state);
}

}    // namespace task_system::detail

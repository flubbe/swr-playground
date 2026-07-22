/**
 * Software Rasterizer Playground.
 *
 * Parallel task graph scheduler.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <future>
#include <memory>

#include "containers/vector.h"
#include "dag.h"

namespace task_system
{

/**
 * Executes a task graph in parallel on the provided thread pool.
 *
 * Resolves task dependencies at runtime, dispatches ready tasks concurrently,
 * and sets the promise value or exception when all tasks finish.
 *
 * @param thread_pool Worker pool used to dispatch tasks.
 * @param state Shared task state for progress and cancellation.
 * @param promise Promise to fulfill on completion or exception.
 * @param tasks Task specifications to execute.
 * @param scheduling_data Pre-computed scheduling metadata for the task graph.
 */
void run_task_specs_scheduler(
  concurrency_utils::deferred_thread_pool<>& thread_pool,
  const std::shared_ptr<TaskSharedState>& state,
  const std::shared_ptr<std::promise<void>>& promise,
  swr::vector<TaskSpec> tasks,
  TaskSchedulingData scheduling_data);

}    // namespace task_system

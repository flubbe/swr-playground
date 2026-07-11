/**
 * Software Rasterizer Playground.
 *
 * Task dependency graph and topological ordering.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <vector>

#include "task_system.h"

namespace task_system
{

/** Scheduling metadata derived from a topologically ordered task graph. */
struct TaskSchedulingData
{
    std::vector<float> weights;
    std::vector<float> task_progress_base;
    std::vector<float> task_progress_span;
    std::vector<std::size_t> indegree;
    std::vector<std::vector<std::size_t>> dependents;
    std::vector<std::size_t> execution_rank;
};

/**
 * Builds a deterministic topological order for the provided task graph.
 *
 * @param tasks Task graph specification list.
 * @returns Dependency-valid execution order as task indices.
 * @throws std::out_of_range if a dependency index is out of range.
 * @throws std::runtime_error if the graph contains a cycle or self-dependency.
 */
[[nodiscard]]
std::vector<std::size_t> build_task_execution_order(
  const std::vector<TaskSpec>& tasks);

/**
 * Computes per-task scheduling metadata from a topologically ordered task list.
 *
 * @param tasks Task list.
 * @param execution_order Dependency-valid execution order produced by build_task_execution_order().
 * @returns Scheduling metadata for the task list.
 */
[[nodiscard]]
TaskSchedulingData build_task_scheduling_data(
  const std::vector<TaskSpec>& tasks,
  const std::vector<std::size_t>& execution_order);

}    // namespace task_system

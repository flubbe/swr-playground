/**
 * Software Rasterizer Playground.
 *
 * Task dependency graph and topological ordering.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "dag.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace task_system
{

namespace
{

/**
 * Return a sorted list (descending) of indices with indegree `0`.
 *
 * @param indegree Indegrees.
 * @returns Returns the `ready` indices into `indegree`.
 */
[[nodiscard]]
std::vector<std::size_t> build_sorted_ready_queue(
  const std::vector<std::size_t>& indegree)
{
    std::vector<std::size_t> ready;
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

}    // namespace

std::vector<std::size_t> build_task_execution_order(
  const std::vector<TaskSpec>& tasks)
{
    const std::size_t task_count = tasks.size();
    std::vector<std::size_t> indegree(task_count, 0);
    std::vector<std::vector<std::size_t>> dependents(task_count);

    for(std::size_t task_index = 0; task_index < task_count; ++task_index)
    {
        const TaskSpec& task = tasks[task_index];
        for(const std::size_t dependency_index: task.dependencies)
        {
            if(dependency_index >= task_count)
            {
                throw std::out_of_range{
                  "Task dependency index out of range"};
            }
            if(dependency_index == task_index)
            {
                throw std::runtime_error{
                  "Task cannot depend on itself"};
            }

            ++indegree[task_index];
            dependents[dependency_index].push_back(task_index);
        }
    }

    std::vector<std::size_t> execution_order;
    execution_order.reserve(task_count);
    std::vector<std::size_t> ready = build_sorted_ready_queue(indegree);

    while(!ready.empty())
    {
        const std::size_t task_index = ready.back();
        ready.pop_back();
        execution_order.push_back(task_index);

        for(const std::size_t dependent_index: dependents[task_index])
        {
            if(indegree[dependent_index] == 0)
            {
                continue;
            }

            --indegree[dependent_index];
            if(indegree[dependent_index] == 0)
            {
                ready.push_back(dependent_index);
            }
        }

        std::ranges::sort(ready, std::greater<>());
    }

    if(execution_order.size() != task_count)
    {
        throw std::runtime_error{
          "Task dependency graph could not be resolved (cycle or unsatisfied dependencies)"};
    }

    return execution_order;
}

TaskSchedulingData build_task_scheduling_data(
  const std::vector<TaskSpec>& tasks,
  const std::vector<std::size_t>& execution_order)
{
    const std::size_t task_count = tasks.size();

    TaskSchedulingData data;

    data.weights.assign(task_count, 1.f);
    float total_weight = 0.f;
    for(std::size_t i = 0; i < task_count; ++i)
    {
        const float weight = std::max(tasks[i].weight, 1.f);
        data.weights[i] = weight;
        total_weight += weight;
    }

    data.task_progress_base.assign(task_count, 0.f);
    data.task_progress_span.assign(task_count, 0.f);
    float consumed_weight = 0.f;
    for(const std::size_t task_index: execution_order)
    {
        data.task_progress_base[task_index] = consumed_weight / total_weight;
        data.task_progress_span[task_index] = data.weights[task_index] / total_weight;
        consumed_weight += data.weights[task_index];
    }

    data.indegree.assign(task_count, 0);
    data.dependents.assign(task_count, {});
    for(std::size_t task_index = 0; task_index < task_count; ++task_index)
    {
        for(const std::size_t dependency_index: tasks[task_index].dependencies)
        {
            ++data.indegree[task_index];
            data.dependents[dependency_index].push_back(task_index);
        }
    }

    data.execution_rank.assign(task_count, 0);
    for(std::size_t position = 0; position < execution_order.size(); ++position)
    {
        data.execution_rank[execution_order[position]] = position;
    }

    return data;
}

}    // namespace task_system

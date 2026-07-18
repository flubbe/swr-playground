/**
 * Software Rasterizer Playground.
 *
 * Task system internal state types and manipulation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace task_system
{

/** Execution state of an individual task snapshot. */
enum class TaskState
{
    Queued,
    Running,
    Skipped,
    Completed,
    Cancelled,
    Failed,
};

/** Snapshot of an individual task's current status and normalized progress. */
struct TaskSnapshot
{
    /** Optional task name associated with the snapshot. */
    std::string name;

    /** Human-readable status string. */
    std::string status_text;

    /** Normalized progress in the range `[0, 1]`. */
    float progress{0.f};

    /** Current execution state of the task. */
    TaskState state{TaskState::Queued};
};

/** Snapshot of a submitted task group's aggregate progress and per-task state. */
struct TaskGroupSnapshot
{
    /** Aggregate normalized progress in the range `[0, 1]`. */
    float progress{0.f};

    /** Current per-task snapshots for the submission. */
    std::vector<TaskSnapshot> tasks;
};

/** Shared state instance used across task execution, handle, and scheduler. */
struct TaskSharedState
{
    /** Protects access to submission snapshots and aggregate progress. */
    mutable std::mutex snapshot_mutex;

    /** Current aggregate submission snapshot exposed through TaskHandle. */
    TaskGroupSnapshot snapshot;

    /** Per-task weights used to compute aggregate submission progress. */
    std::vector<float> task_weights;

    /** Sum of normalized task weights. */
    float total_weight{0.f};

    /** Set to true to request cancellation for running tasks. */
    std::atomic_bool cancel_requested{false};

    /** Set to true when execution has fully finished. */
    std::atomic_bool finished{false};

    /** Protects wait/notify sequencing for completion. */
    mutable std::mutex finished_mutex;

    /** Notifies waiters when task execution is complete. */
    std::condition_variable finished_cv;
};

// Internal implementation details (not part of public API).
namespace detail
{

/**
 * Update the progress of a task group.
 *
 * @note The snapshot mutex must be held by the caller.
 *
 * @param state Task shared state.
 */
void refresh_group_progress_locked(
  TaskSharedState& state);

/**
 * Update task execution state and status.
 *
 * @param state Shared task state.
 * @param task_index Task index to update.
 * @param task_state New task state.
 * @param status_text Optional new status text.
 * @param progress Optional new progress value.
 */
void set_task_state(
  TaskSharedState& state,
  std::size_t task_index,
  TaskState task_state,
  std::optional<std::string> status_text = std::nullopt,
  std::optional<float> progress = std::nullopt);

/**
 * Mark all queued and running tasks as cancelled.
 *
 * @param state Shared task state.
 */
void cancel_unfinished_tasks(
  TaskSharedState& state);

}    // namespace detail

}    // namespace task_system

# Task System

This module provides asynchronous task execution with dependency-aware scheduling, cooperative cancellation, and progress snapshots.

## Components

- `dag.h/.cpp`: task graph validation and deterministic topological ordering.
- `scheduler.h/.cpp`: parallel DAG scheduler that dispatches ready tasks and propagates failures/cancellation.
- `state.h/.cpp`: shared snapshot/cancellation/completion state and internal state update helpers.
- `task_logger.h`: logging abstraction (`TaskLogger`) plus no-op default (`NullTaskLogger`).
- `task_system.h/.cpp`: public API (`TaskSystem`, `TaskExecutionContext`, `TaskHandle`, `TaskSubmission`, `TaskSpec`).

## Submission Model

`TaskSystem` supports two submission paths:

- `submit(Fn&&)`: submits one callable that receives `TaskExecutionContext&`.
- `submit_task_specs(swr::vector<TaskSpec>)`: submits a dependency graph.

Both return `TaskSubmission<Result>`:

- `handle`: polling/cancellation/wait access.
- `future`: completion/error propagation.

Multiple submissions may run concurrently. Use `future.get()` if explicit sequencing is required.

## Task Graph Semantics

For `submit_task_specs(...)`:

- Dependencies are declared by task indices (`TaskSpec::dependencies`).
- Submission validates the graph and freezes a deterministic topological order.
- Runtime scheduling is parallel: all currently ready tasks may run concurrently.
- `start_condition == false` marks the task as `Skipped` and still unblocks dependents.
- First task failure is propagated and triggers cooperative cancellation of remaining work.
- Dependents of failed/cancelled tasks transition to `Cancelled`.

Validation failures:

- Out-of-range dependency index -> `std::out_of_range`.
- Self-dependency or cycle -> `std::runtime_error`.

## Progress and Snapshot Model

Progress is tracked per task and for the aggregate submission:

- `TaskExecutionContext::update(status, progress)` updates task-local status/progress.
- Aggregate progress is weighted by `TaskSpec::weight` (minimum effective weight is `1.0`).
- `TaskHandle::snapshot()` returns a `TaskGroupSnapshot` with:
  - `progress` in `[0, 1]`
  - per-task `TaskSnapshot` entries (`name`, `status_text`, `progress`, `state`)

Task states:

- `Queued`
- `Running`
- `Skipped`
- `Completed`
- `Cancelled`
- `Failed`

## Cancellation Model

Cancellation is cooperative:

- `TaskHandle::cancel()` sets a shared cancel flag.
- Tasks should call `TaskExecutionContext::is_cancel_requested()` and stop promptly.
- Throwing `TaskCancelledError` signals cooperative cancellation explicitly.

`TaskHandle::wait()` blocks until submission completion is signaled.

## Logging Integration

The task module is decoupled from application logging by `TaskLogger`:

- `TaskSystem(worker_count)` defaults to `NullTaskLogger::instance()` (no output).
- To enable logs, provide an adapter that implements `TaskLogger`.

Example adapter pattern:

```cpp
class AppTaskLogger final : public task_system::TaskLogger
{
    const logging::Logger logger;

public:
    explicit AppTaskLogger(logging::LogDevice& device)
    : logger{"TaskSystem", device}
    {
    }

    void log(std::string_view msg) const override { logger.logf("{}", msg); }
    void warn(std::string_view msg) const override { logger.warningf("{}", msg); }
    void error(std::string_view msg) const override { logger.errorf("{}", msg); }
};
```

## Minimal Usage

```cpp
task_system::TaskSystem tasks{4};

swr::vector<task_system::TaskSpec> graph;
graph.push_back(task_system::TaskSpec{
  .name = "Load A",
  .weight = 1.f,
  .run = [](task_system::TaskExecutionContext& ctx)
  {
      ctx.update("Loading A", 0.5f);
      ctx.update("Loading A", 1.f);
  },
});

graph.push_back(task_system::TaskSpec{
  .name = "Finalize",
  .weight = 1.f,
  .dependencies = {0},
  .run = [](task_system::TaskExecutionContext& ctx)
  {
      ctx.update("Finalizing", 1.f);
  },
});

auto submission = tasks.submit_task_specs(std::move(graph));
submission.future.get();

const task_system::TaskGroupSnapshot snapshot = submission.handle.snapshot();
```

## Notes

- `TaskExecutionContext::subrange(start, scale)` supports nested progress partitioning.
- `submit(Fn&&)` stores a single synthetic task snapshot in the returned handle.
- For reproducible rank/order metadata, graph order is frozen at submission time; runtime execution remains parallel for ready nodes.

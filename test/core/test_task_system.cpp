#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <gtest/gtest.h>

#include "containers/vector.h"
#include "tasks/task_system.h"

namespace
{

using task_system::TaskCancelledError;
using task_system::TaskExecutionContext;
using task_system::TaskGroupSnapshot;
using task_system::TaskSpec;
using task_system::TaskState;
using task_system::TaskSystem;

void update_maximum(
  std::atomic<int>& target,
  int value)
{
    int current = target.load(std::memory_order_relaxed);
    while(current < value
          && !target.compare_exchange_weak(
            current,
            value,
            std::memory_order_relaxed,
            std::memory_order_relaxed))
    {
    }
}

}    // namespace

TEST(TaskSystemTests, SubmitTaskSpecsRunsIndependentTasksInParallelAndFinalizerWaits)
{
    using namespace std::literals;

    TaskSystem task_system{4};

    std::atomic<int> running_count{0};
    std::atomic<int> max_running_count{0};
    std::atomic<int> ready_count{0};

    std::atomic<bool> task_a_done{false};
    std::atomic<bool> task_b_done{false};
    std::atomic<bool> final_ran{false};
    std::atomic<bool> final_started_before_dependencies{false};

    std::mutex order_mutex;
    swr::vector<std::size_t> execution_order;

    auto parallel_branch =
      [&](TaskExecutionContext& context, std::size_t branch_index, std::atomic<bool>& done_flag)
    {
        const int running_now = running_count.fetch_add(1, std::memory_order_relaxed) + 1;
        update_maximum(max_running_count, running_now);
        ready_count.fetch_add(1, std::memory_order_relaxed);

        const auto deadline = std::chrono::steady_clock::now() + 500ms;
        while(ready_count.load(std::memory_order_relaxed) < 2)
        {
            if(context.is_cancel_requested())
            {
                throw TaskCancelledError{};
            }
            if(std::chrono::steady_clock::now() > deadline)
            {
                throw std::runtime_error{"Parallel branches did not overlap"};
            }
            std::this_thread::sleep_for(1ms);
        }

        {
            std::scoped_lock lock{order_mutex};
            execution_order.push_back(branch_index);
        }

        std::this_thread::sleep_for(20ms);
        done_flag.store(true, std::memory_order_relaxed);
        running_count.fetch_sub(1, std::memory_order_relaxed);
    };

    swr::vector<TaskSpec> tasks;
    tasks.reserve(3);

    tasks.push_back(TaskSpec{
      .name = "Branch A",
      .weight = 1.f,
      .run = [&](TaskExecutionContext& context)
      {
          parallel_branch(
            context,
            0,
            task_a_done);
      },
    });

    tasks.push_back(TaskSpec{
      .name = "Branch B",
      .weight = 1.f,
      .run = [&](TaskExecutionContext& context)
      {
          parallel_branch(
            context,
            1,
            task_b_done);
      },
    });

    tasks.push_back(TaskSpec{
      .name = "Finalize",
      .weight = 1.f,
      .dependencies = {0, 1},
      .run = [&](TaskExecutionContext&)
      {
          if(!task_a_done.load(std::memory_order_relaxed)
             || !task_b_done.load(std::memory_order_relaxed))
          {
              final_started_before_dependencies.store(true, std::memory_order_relaxed);
          }

          {
              std::scoped_lock lock{order_mutex};
              execution_order.push_back(2);
          }
          final_ran.store(true, std::memory_order_relaxed);
      },
    });

    auto submission = task_system.submit_task_specs(std::move(tasks));
    ASSERT_TRUE(submission.future.valid());

    EXPECT_NO_THROW(submission.future.get());
    EXPECT_TRUE(final_ran.load(std::memory_order_relaxed));
    EXPECT_FALSE(final_started_before_dependencies.load(std::memory_order_relaxed));
    EXPECT_GE(max_running_count.load(std::memory_order_relaxed), 2);

    const TaskGroupSnapshot snapshot = submission.handle.snapshot();
    ASSERT_EQ(snapshot.tasks.size(), 3u);
    EXPECT_FLOAT_EQ(snapshot.progress, 1.f);
    EXPECT_EQ(snapshot.tasks[0].state, TaskState::Completed);
    EXPECT_EQ(snapshot.tasks[1].state, TaskState::Completed);
    EXPECT_EQ(snapshot.tasks[2].state, TaskState::Completed);

    ASSERT_EQ(execution_order.size(), 3u);
    EXPECT_EQ(execution_order.back(), 2u);
}

TEST(TaskSystemTests, SubmitTaskSpecsPropagatesFirstExceptionAndCancelsOtherBranches)
{
    using namespace std::literals;

    TaskSystem task_system{4};

    std::atomic<bool> branch_started{false};
    std::atomic<bool> branch_observed_cancel{false};
    std::atomic<bool> branch_finished_normally{false};
    std::atomic<bool> finalizer_ran{false};

    swr::vector<TaskSpec> tasks;
    tasks.reserve(3);

    tasks.push_back(TaskSpec{
      .name = "Failing Branch",
      .weight = 1.f,
      .run = [&](TaskExecutionContext&)
      {
          const auto deadline =
            std::chrono::steady_clock::now() + 500ms;
          while(!branch_started.load(std::memory_order_relaxed))
          {
              if(std::chrono::steady_clock::now() > deadline)
              {
                  throw std::runtime_error{"Timed out waiting for sibling branch"};
              }
              std::this_thread::sleep_for(1ms);
          }

          throw std::runtime_error{"Expected task failure"};
      },
    });

    tasks.push_back(TaskSpec{
      .name = "Long Running Branch",
      .weight = 1.f,
      .run = [&](TaskExecutionContext& context)
      {
          branch_started.store(true, std::memory_order_relaxed);

          const auto deadline =
            std::chrono::steady_clock::now() + 1s;
          while(std::chrono::steady_clock::now() < deadline)
          {
              if(context.is_cancel_requested())
              {
                  branch_observed_cancel.store(true, std::memory_order_relaxed);
                  throw TaskCancelledError{};
              }

              std::this_thread::sleep_for(2ms);
          }

          branch_finished_normally.store(true, std::memory_order_relaxed);
      },
    });

    tasks.push_back(TaskSpec{
      .name = "Finalizer",
      .weight = 1.f,
      .dependencies = {0, 1},
      .run = [&](TaskExecutionContext&)
      {
          finalizer_ran.store(true, std::memory_order_relaxed);
      },
    });

    auto submission = task_system.submit_task_specs(std::move(tasks));
    ASSERT_TRUE(submission.future.valid());

    try
    {
        submission.future.get();
        FAIL() << "Expected runtime_error from failing branch";
    }
    catch(const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Expected task failure");
    }
    catch(...)
    {
        FAIL() << "Expected std::runtime_error";
    }

    const auto cancel_deadline =
      std::chrono::steady_clock::now() + 300ms;
    while(!branch_observed_cancel.load(std::memory_order_relaxed)
          && !branch_finished_normally.load(std::memory_order_relaxed)
          && std::chrono::steady_clock::now() < cancel_deadline)
    {
        std::this_thread::sleep_for(1ms);
    }

    EXPECT_TRUE(branch_started.load(std::memory_order_relaxed));
    EXPECT_TRUE(branch_observed_cancel.load(std::memory_order_relaxed));
    EXPECT_FALSE(branch_finished_normally.load(std::memory_order_relaxed));
    EXPECT_FALSE(finalizer_ran.load(std::memory_order_relaxed));

    const TaskGroupSnapshot snapshot = submission.handle.snapshot();
    ASSERT_EQ(snapshot.tasks.size(), 3u);
    EXPECT_EQ(snapshot.tasks[0].state, TaskState::Failed);
    EXPECT_EQ(snapshot.tasks[1].state, TaskState::Cancelled);
}

TEST(TaskSystemTests, SubmitTaskSpecsSkippedTaskUnblocksDependents)
{
    TaskSystem task_system{4};

    std::atomic<bool> skipped_task_ran{false};
    std::atomic<bool> dependency_ran{false};
    std::atomic<bool> dependent_ran{false};

    std::mutex order_mutex;
    swr::vector<std::size_t> execution_order;

    swr::vector<TaskSpec> tasks;
    tasks.reserve(3);

    tasks.push_back(TaskSpec{
      .name = "Skipped Root",
      .weight = 1.f,
      .start_condition = []()
      { return false; },
      .run = [&](TaskExecutionContext&)
      { skipped_task_ran.store(true, std::memory_order_relaxed); },
    });

    tasks.push_back(TaskSpec{
      .name = "Normal Dependency",
      .weight = 1.f,
      .run = [&](TaskExecutionContext&)
      {
          dependency_ran.store(true, std::memory_order_relaxed);
          std::scoped_lock lock{order_mutex};
          execution_order.push_back(1);
      },
    });

    tasks.push_back(TaskSpec{
      .name = "Dependent",
      .weight = 1.f,
      .dependencies = {0, 1},
      .run = [&](TaskExecutionContext&)
      {
          dependent_ran.store(true, std::memory_order_relaxed);
          std::scoped_lock lock{order_mutex};
          execution_order.push_back(2);
      },
    });

    auto submission = task_system.submit_task_specs(std::move(tasks));
    ASSERT_TRUE(submission.future.valid());

    EXPECT_NO_THROW(submission.future.get());
    EXPECT_FALSE(skipped_task_ran.load(std::memory_order_relaxed));
    EXPECT_TRUE(dependency_ran.load(std::memory_order_relaxed));
    EXPECT_TRUE(dependent_ran.load(std::memory_order_relaxed));

    const TaskGroupSnapshot snapshot = submission.handle.snapshot();
    ASSERT_EQ(snapshot.tasks.size(), 3u);
    EXPECT_EQ(snapshot.tasks[0].state, TaskState::Skipped);
    EXPECT_EQ(snapshot.tasks[1].state, TaskState::Completed);
    EXPECT_EQ(snapshot.tasks[2].state, TaskState::Completed);
    EXPECT_FLOAT_EQ(snapshot.progress, 1.f);

    ASSERT_EQ(execution_order.size(), 2u);
    EXPECT_EQ(execution_order[0], 1u);
    EXPECT_EQ(execution_order[1], 2u);
}

TEST(TaskSystemTests, SubmitTaskSpecsFailureDoesNotPoisonSubsequentSubmissions)
{
    TaskSystem task_system{4};

    std::atomic<bool> first_task_ran{false};
    std::atomic<bool> trailing_task_ran{false};
    std::atomic<bool> second_submission_task_ran{false};

    swr::vector<TaskSpec> failing_tasks;
    failing_tasks.reserve(3);

    failing_tasks.push_back(TaskSpec{
      .name = "First Task",
      .weight = 1.f,
      .run = [&](TaskExecutionContext&)
      {
          first_task_ran.store(true, std::memory_order_relaxed);
      },
    });

    failing_tasks.push_back(TaskSpec{
      .name = "Failing Task",
      .weight = 1.f,
      .dependencies = {0},
      .run = [&](TaskExecutionContext&)
      {
          throw std::runtime_error{"Synthetic failure in first submission"};
      },
    });

    failing_tasks.push_back(TaskSpec{
      .name = "Trailing Task",
      .weight = 1.f,
      .dependencies = {1},
      .run = [&](TaskExecutionContext&)
      {
          trailing_task_ran.store(true, std::memory_order_relaxed);
      },
    });

    auto failing_submission = task_system.submit_task_specs(std::move(failing_tasks));
    ASSERT_TRUE(failing_submission.future.valid());

    try
    {
        failing_submission.future.get();
        FAIL() << "Expected runtime_error from failing task";
    }
    catch(const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Synthetic failure in first submission");
    }
    catch(...)
    {
        FAIL() << "Expected std::runtime_error";
    }

    EXPECT_TRUE(first_task_ran.load(std::memory_order_relaxed));
    EXPECT_FALSE(trailing_task_ran.load(std::memory_order_relaxed));

    const TaskGroupSnapshot failing_snapshot = failing_submission.handle.snapshot();
    ASSERT_EQ(failing_snapshot.tasks.size(), 3u);
    EXPECT_EQ(failing_snapshot.tasks[0].state, TaskState::Completed);
    EXPECT_EQ(failing_snapshot.tasks[1].state, TaskState::Failed);
    EXPECT_EQ(failing_snapshot.tasks[2].state, TaskState::Cancelled);

    swr::vector<TaskSpec> succeeding_tasks;
    succeeding_tasks.push_back(TaskSpec{
      .name = "Recovery Task",
      .weight = 1.f,
      .run = [&](TaskExecutionContext&)
      {
          second_submission_task_ran.store(true, std::memory_order_relaxed);
      },
    });

    auto succeeding_submission = task_system.submit_task_specs(std::move(succeeding_tasks));
    ASSERT_TRUE(succeeding_submission.future.valid());

    EXPECT_NO_THROW(succeeding_submission.future.get());
    EXPECT_TRUE(second_submission_task_ran.load(std::memory_order_relaxed));

    const TaskGroupSnapshot succeeding_snapshot = succeeding_submission.handle.snapshot();
    ASSERT_EQ(succeeding_snapshot.tasks.size(), 1u);
    EXPECT_EQ(succeeding_snapshot.tasks[0].state, TaskState::Completed);
    EXPECT_FLOAT_EQ(succeeding_snapshot.progress, 1.f);
}

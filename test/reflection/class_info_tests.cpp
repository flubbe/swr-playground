#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

#include <gtest/gtest.h>

#include "reflection/class_info.h"

namespace
{

const reflect::ClassInfo* g_manual_resolved_super = nullptr;
int g_manual_super_resolve_calls = 0;
reflect::ClassInfo* g_cycle_class = nullptr;
reflect::ClassInfo* g_cycle_class_a = nullptr;
reflect::ClassInfo* g_cycle_class_b = nullptr;
std::atomic<int> g_retry_super_resolve_calls = 0;
std::atomic<int> g_concurrent_super_resolve_calls = 0;

const reflect::ClassInfo* resolve_manual_super()
{
    ++g_manual_super_resolve_calls;
    return g_manual_resolved_super;
}

const reflect::ClassInfo* resolve_cycle_super()
{
    return g_cycle_class->get_super();
}

const reflect::ClassInfo* resolve_cycle_super_a()
{
    return g_cycle_class_b->get_super();
}

const reflect::ClassInfo* resolve_cycle_super_b()
{
    return g_cycle_class_a->get_super();
}

const reflect::ClassInfo* resolve_super_retry_once()
{
    if(++g_retry_super_resolve_calls == 1)
    {
        throw std::runtime_error("Simulated first-resolution failure");
    }

    return g_manual_resolved_super;
}

const reflect::ClassInfo* resolve_super_with_delay()
{
    ++g_concurrent_super_resolve_calls;
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    return g_manual_resolved_super;
}

}    // namespace

TEST(ClassInfoTests, LazilyResolvesAndCachesSuperClass)
{
    reflect::ClassInfo super{};
    super.qualified_name = "Test.Super";

    reflect::ClassInfo child{};
    child.qualified_name = "Test.Child";
    child.resolve_super = &resolve_manual_super;

    g_manual_resolved_super = &super;
    g_manual_super_resolve_calls = 0;

    EXPECT_EQ(child.get_super(), &super);
    EXPECT_EQ(child.get_super(), &super);
    EXPECT_EQ(g_manual_super_resolve_calls, 1);
}

TEST(ClassInfoTests, ThrowsOnCircularSuperClassResolution)
{
    reflect::ClassInfo cyclic{};
    cyclic.qualified_name = "Test.Cyclic";
    cyclic.resolve_super = &resolve_cycle_super;

    g_cycle_class = &cyclic;
    EXPECT_THROW(cyclic.get_super(), std::runtime_error);
    g_cycle_class = nullptr;
}

TEST(ClassInfoTests, ThrowsOnIndirectCircularSuperClassResolution)
{
    reflect::ClassInfo class_a{};
    class_a.qualified_name = "Test.CycleA";
    class_a.resolve_super = &resolve_cycle_super_a;

    reflect::ClassInfo class_b{};
    class_b.qualified_name = "Test.CycleB";
    class_b.resolve_super = &resolve_cycle_super_b;

    g_cycle_class_a = &class_a;
    g_cycle_class_b = &class_b;

    EXPECT_THROW(class_a.get_super(), std::runtime_error);
    EXPECT_THROW(class_b.get_super(), std::runtime_error);

    g_cycle_class_a = nullptr;
    g_cycle_class_b = nullptr;
}

TEST(ClassInfoTests, RetriesSuperResolutionAfterResolverFailure)
{
    reflect::ClassInfo super{};
    super.qualified_name = "Test.Super";

    reflect::ClassInfo child{};
    child.qualified_name = "Test.ChildRetry";
    child.resolve_super = &resolve_super_retry_once;

    g_manual_resolved_super = &super;
    g_retry_super_resolve_calls = 0;

    EXPECT_THROW(child.get_super(), std::runtime_error);
    EXPECT_EQ(child.get_super(), &super);
    EXPECT_EQ(g_retry_super_resolve_calls.load(), 2);
}

TEST(ClassInfoTests, ConcurrentGetSuperResolvesOnlyOnce)
{
    reflect::ClassInfo super{};
    super.qualified_name = "Test.Super";

    reflect::ClassInfo child{};
    child.qualified_name = "Test.ChildConcurrent";
    child.resolve_super = &resolve_super_with_delay;

    g_manual_resolved_super = &super;
    g_concurrent_super_resolve_calls = 0;

    std::atomic<bool> start{false};
    const reflect::ClassInfo* first = nullptr;
    const reflect::ClassInfo* second = nullptr;

    std::thread t1(
      [&]
      {
          while(!start.load())
          {
          }
          first = child.get_super();
      });
    std::thread t2(
      [&]
      {
          while(!start.load())
          {
          }
          second = child.get_super();
      });

    start = true;
    t1.join();
    t2.join();

    EXPECT_EQ(first, &super);
    EXPECT_EQ(second, &super);
    EXPECT_EQ(g_concurrent_super_resolve_calls.load(), 1);
}

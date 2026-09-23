#include <gtest/gtest.h>

#define SWR_USE_CUSTOM_STD_ALLOCATORS 1

#include "containers/memory.h"
#include "containers/deque.h"
#include "containers/string.h"
#include "containers/unordered_map.h"
#include "containers/unordered_set.h"
#include "containers/vector.h"
#include "memory/manager.h"

class MemoryManagerTests
: public ::testing::Test
{
protected:
    void SetUp() override
    {
        memory::initialize();
    }

    void TearDown() override
    {
        memory::shutdown();
    }
};

TEST_F(MemoryManagerTests, InitialStats)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.allocate_calls, 1);    // frame bump is allocated
    EXPECT_EQ(stats.bytes_live, memory::default_bump_size);
    EXPECT_EQ(stats.bytes_peak, memory::default_bump_size);
    EXPECT_EQ(stats.bytes_total_allocated, memory::default_bump_size);
    EXPECT_EQ(stats.deallocate_calls, 0);

    for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
    {
        if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
        {
            EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
        }
        else
        {
            EXPECT_EQ(stats.bytes_per_tag[i], 0);
        }
    }
}

TEST_F(MemoryManagerTests, Deque)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::Deque)], 0);

    {
        auto ptr = swr::deque<int>{};
        ptr.push_back(42);

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        EXPECT_GT(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::Deque)], 0);
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::Deque)], 0);
}

TEST_F(MemoryManagerTests, UniquePtr)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UniquePtr)], 0);

    {
        auto ptr = swr::make_unique<int>(42);

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        EXPECT_GT(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UniquePtr)], 0);
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UniquePtr)], 0);
}

TEST_F(MemoryManagerTests, String)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::String)], 0);

    {
        auto ptr = swr::string{"LongStringToTriggerAllocation"};

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        EXPECT_GT(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::String)], 0);
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::String)], 0);
}

TEST_F(MemoryManagerTests, UnorderedMap)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedMap)], 0);

    {
        auto ptr = swr::unordered_map<int, int>{
          {1, 2}};

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        EXPECT_GT(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedMap)], 0);
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedMap)], 0);
}

TEST_F(MemoryManagerTests, UnorderedSet)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedSet)], 0);

    {
        auto ptr = swr::unordered_set<int>{1, 2};

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        EXPECT_GT(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedSet)], 0);
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedSet)], 0);
}

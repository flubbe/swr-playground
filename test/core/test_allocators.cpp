#include <gtest/gtest.h>

#include <string_view>

#include "memory/allocators/arena.h"
#include "memory/allocators/bump.h"
#include "memory/allocators/malloc.h"

TEST(MallocAllocatorTests, Name)
{
    auto allocator = memory::MallocAllocator();
    std::string name = allocator.name();

    EXPECT_EQ(name, "Malloc");
}

TEST(MallocAllocatorTests, AllocateZeroBytes)
{
    memory::MallocAllocator allocator;

    void* p = allocator.allocate(
      0,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p, nullptr);
    allocator.deallocate(
      p,
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
}

TEST(MallocAllocatorTests, AllocateOneByte)
{
    memory::MallocAllocator allocator;

    void* p = allocator.allocate(
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::Bump);
    ASSERT_NE(p, nullptr);
    allocator.deallocate(
      p,
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
}

TEST(MallocAllocatorTests, Alignment)
{
    memory::MallocAllocator allocator;

    constexpr std::size_t alignments[] = {
      1, 2, 4, 8, 16, 32, 64, 128};

    for(auto alignment: alignments)
    {
        void* p = allocator.allocate(
          1,
          alignment,
          memory::MemoryTag::None);

        ASSERT_NE(p, nullptr);

        EXPECT_EQ(
          reinterpret_cast<std::uintptr_t>(p) % alignment,
          0u);

        allocator.deallocate(
          p,
          1,
          alignment,
          memory::MemoryTag::None);
    }
}

TEST(MallocAllocatorTests, ReadWrite)
{
    memory::MallocAllocator allocator;

    constexpr std::size_t bytes = 128;

    auto* p = static_cast<std::uint8_t*>(
      allocator.allocate(
        bytes,
        alignof(std::uint64_t),
        memory::MemoryTag::None));

    ASSERT_NE(p, nullptr);

    for(std::size_t i = 0; i < bytes; ++i)
    {
        p[i] = static_cast<std::uint8_t>(i);
    }

    for(std::size_t i = 0; i < bytes; ++i)
    {
        EXPECT_EQ(p[i], static_cast<std::uint8_t>(i));
    }

    allocator.deallocate(
      p,
      bytes,
      alignof(std::uint64_t),
      memory::MemoryTag::None);
}

TEST(MallocAllocatorTests, MultipleAllocations)
{
    memory::MallocAllocator allocator;

    auto* a = static_cast<std::uint8_t*>(
      allocator.allocate(64, 16, memory::MemoryTag::None));
    auto* b = static_cast<std::uint8_t*>(
      allocator.allocate(64, 16, memory::MemoryTag::None));

    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    EXPECT_NE(a, b);

    std::fill(a, a + 64, 0xAA);
    std::fill(b, b + 64, 0x55);

    EXPECT_EQ(a[0], 0xAA);
    EXPECT_EQ(b[0], 0x55);

    allocator.deallocate(a, 64, 16, memory::MemoryTag::None);
    allocator.deallocate(b, 64, 16, memory::MemoryTag::None);
}

TEST(MallocAllocatorTests, LargeAllocation)
{
    memory::MallocAllocator allocator;

    constexpr std::size_t bytes = 32 * 1024 * 1024;

    void* p = allocator.allocate(bytes, 64, memory::MemoryTag::None);

    ASSERT_NE(p, nullptr);

    allocator.deallocate(p, bytes, 64, memory::MemoryTag::None);
}

TEST(MallocAllocatorTests, Stress)
{
    memory::MallocAllocator allocator;

    for(int i = 0; i < 10000; ++i)
    {
        void* p = allocator.allocate(64, 16, memory::MemoryTag::None);

        ASSERT_NE(p, nullptr);

        allocator.deallocate(p, 64, 16, memory::MemoryTag::None);
    }
}

TEST(BumpAllocatorTests, Name)
{
    memory::MallocAllocator upstream;
    memory::BumpAllocator allocator{1024, upstream};
    std::string name = allocator.name();

    EXPECT_EQ(name, "Bump");
}

TEST(BumpAllocatorTests, InvalidBackingMemorySize)
{
    memory::MallocAllocator upstream;
    ASSERT_THROW(
      memory::BumpAllocator(0, upstream),
      std::invalid_argument);
}

TEST(BumpAllocatorTests, SequentialAllocations)
{
    memory::MallocAllocator upstream;
    memory::BumpAllocator allocator{1024, upstream};

    auto* a = allocator.allocate(16, 8, memory::MemoryTag::None);
    auto* b = allocator.allocate(16, 8, memory::MemoryTag::None);

    EXPECT_LT(a, b);
}

TEST(BumpAllocatorTests, OutOfMemory)
{
    memory::MallocAllocator upstream;
    memory::BumpAllocator allocator{64, upstream};

    EXPECT_NE(allocator.allocate(64, 1, memory::MemoryTag::None), nullptr);

    EXPECT_THROW(
      (void)allocator.allocate(1, 1, memory::MemoryTag::None),
      std::bad_alloc);
}

TEST(BumpAllocatorTests, ResetReusesMemory)
{
    memory::MallocAllocator upstream;
    memory::BumpAllocator allocator{128, upstream};

    void* first = allocator.allocate(32, 8, memory::MemoryTag::None);

    allocator.reset();

    void* second = allocator.allocate(32, 8, memory::MemoryTag::None);

    EXPECT_EQ(first, second);
}

TEST(BumpAllocatorTests, DeallocateNull)
{
    memory::MallocAllocator upstream;
    memory::BumpAllocator allocator{128, upstream};

    EXPECT_NO_THROW(
      allocator.deallocate(
        nullptr,
        1,
        1,
        memory::MemoryTag::None));
}

TEST(BumpAllocatorTests, Alignment)
{
    memory::MallocAllocator upstream;
    memory::BumpAllocator allocator{256, upstream};

    constexpr std::size_t alignments[] = {
      1, 2, 4, 8, 16, 32, 64, 128};

    for(auto alignment: alignments)
    {
        void* p = allocator.allocate(1, alignment, memory::MemoryTag::None);

        ASSERT_NE(p, nullptr);

        EXPECT_EQ(
          reinterpret_cast<std::uintptr_t>(p) % alignment,
          0u);

        allocator.deallocate(p, 1, alignment, memory::MemoryTag::None);
    }
}

TEST(ArenaAllocatorTests, Name)
{
    memory::MallocAllocator upstream;
    auto allocator = memory::ArenaAllocator{upstream};

    std::string name = allocator.name();

    EXPECT_EQ(name, "Arena");
}

TEST(ArenaAllocatorTests, AllocateZeroBytes)
{
    memory::MallocAllocator upstream;
    auto allocator = memory::ArenaAllocator{upstream};

    void* p = allocator.allocate(
      0,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p, nullptr);
    allocator.deallocate(
      p,
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
}

TEST(ArenaAllocatorTests, AllocateOneByte)
{
    memory::MallocAllocator upstream;
    auto allocator = memory::ArenaAllocator{upstream};

    void* p = allocator.allocate(
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p, nullptr);
    allocator.deallocate(
      p,
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
}

TEST(ArenaAllocatorTests, AllocateOneByteReset)
{
    memory::MallocAllocator upstream;
    auto allocator = memory::ArenaAllocator{upstream};

    void* p = allocator.allocate(
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p, nullptr);
    allocator.deallocate(
      p,
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);

    allocator.reset();

    auto stats = allocator.get_stats();
    EXPECT_EQ(stats.allocations, 1);
    EXPECT_EQ(stats.deallocations, 1);
    EXPECT_EQ(stats.pages, 1);
}

TEST(ArenaAllocatorTests, AllocateTwoPages)
{
    memory::MallocAllocator upstream;
    auto allocator = memory::ArenaAllocator{upstream, 1};

    void* p1 = allocator.allocate(
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p1, nullptr);

    void* p2 = allocator.allocate(
      128,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p2, nullptr);

    EXPECT_NE(p1, p2);

    allocator.deallocate(
      p2,
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    allocator.deallocate(
      p1,
      128,
      alignof(std::max_align_t),
      memory::MemoryTag::None);

    allocator.reset();

    auto stats = allocator.get_stats();
    EXPECT_EQ(stats.allocations, 2);
    EXPECT_EQ(stats.deallocations, 2);
    EXPECT_EQ(stats.pages, 2);
}

TEST(ArenaAllocatorTests, ReusePages)
{
    memory::MallocAllocator upstream;
    auto allocator = memory::ArenaAllocator{upstream, 1};

    // allocate two pages
    void* p1 = allocator.allocate(
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p1, nullptr);

    void* p2 = allocator.allocate(
      128,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p2, nullptr);

    EXPECT_NE(p1, p2);

    allocator.deallocate(
      p2,
      1,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    allocator.deallocate(
      p1,
      128,
      alignof(std::max_align_t),
      memory::MemoryTag::None);

    // reset and allocate memory fitting into the second page
    allocator.reset();

    p1 = allocator.allocate(
      64,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p1, nullptr);

    p2 = allocator.allocate(
      32,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    ASSERT_NE(p2, nullptr);

    EXPECT_NE(p1, p2);

    allocator.deallocate(
      p2,
      64,
      alignof(std::max_align_t),
      memory::MemoryTag::None);
    allocator.deallocate(
      p1,
      32,
      alignof(std::max_align_t),
      memory::MemoryTag::None);

    EXPECT_EQ(allocator.size(), 96);

    auto stats = allocator.get_stats();
    EXPECT_EQ(stats.allocations, 2);
    EXPECT_EQ(stats.deallocations, 2);
    EXPECT_EQ(stats.pages, 2);
}

TEST(ArenaAllocatorTests, Alignment)
{
    memory::MallocAllocator upstream;
    auto allocator = memory::ArenaAllocator{upstream, 64};

    constexpr std::size_t alignments[] = {
      1, 2, 4, 8, 16, 32, 64, 128};

    for(auto alignment: alignments)
    {
        void* p = allocator.allocate(
          1,
          alignment,
          memory::MemoryTag::None);

        ASSERT_NE(p, nullptr);

        EXPECT_EQ(
          reinterpret_cast<std::uintptr_t>(p) % alignment,
          0u);

        allocator.deallocate(
          p,
          1,
          alignment,
          memory::MemoryTag::None);
    }
}

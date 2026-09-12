/**
 * Software Rasterizer Playground.
 *
 * Bump allocator.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <new>
#include <stdexcept>

#include "memory/allocator.h"

namespace memory
{

/** Bump allocator statistics. */
struct BumpAllocatorStats
{
    std::size_t allocations = 0;
    std::size_t deallocations = 0;
    std::size_t used_before_reset = 0;
    std::size_t used_peak = 0;
    std::size_t free_before_reset = 0;
};

/** Bump allocator. */
class BumpAllocator final
: public Allocator
{
    const std::size_t alignment{alignof(std::max_align_t)};

    Allocator* allocator{nullptr};

    void* memory{nullptr};
    void* end{nullptr};

    std::atomic<void*> base{nullptr};

    std::atomic_size_t allocations{0};
    std::atomic_size_t deallocations{0};
    BumpAllocatorStats stats;

public:
    BumpAllocator(
      std::size_t bytes,
      Allocator* allocator)
    : allocator{allocator}
    {
        if(bytes == 0)
        {
            throw std::invalid_argument{
              "BumpAllocator size must be greater than zero"};
        }

        if(allocator == nullptr)
        {
            throw std::invalid_argument{
              "allocator must not be null"};
        }

        memory = allocator->allocate(
          bytes,
          alignment);
        if(memory == nullptr)
        {
            throw std::bad_alloc{};
        }

        end = reinterpret_cast<std::byte*>(memory) + bytes;
        base = memory;
    }
    ~BumpAllocator()
    {
        allocator->deallocate(
          memory,
          capacity(),
          alignment);
    }

    void reset() noexcept
    {
        stats.allocations = allocations.load(std::memory_order::relaxed);
        stats.deallocations = deallocations.load(std::memory_order::relaxed);
        stats.used_before_reset = size();
        stats.used_peak = std::max(stats.used_peak, stats.used_before_reset);
        stats.free_before_reset = capacity() - stats.used_before_reset;

        base = memory;
        allocations = 0;
        deallocations = 0;
    }

    std::size_t size() const noexcept
    {
        return reinterpret_cast<const std::byte*>(
                 base.load(std::memory_order::relaxed))
               - reinterpret_cast<const std::byte*>(memory);
    }

    std::size_t capacity() const noexcept
    {
        return reinterpret_cast<const std::byte*>(end)
               - reinterpret_cast<const std::byte*>(memory);
    }

    BumpAllocatorStats get_stats() const noexcept
    {
        return stats;
    }

    [[nodiscard]]
    void* allocate(
      std::size_t bytes,
      std::size_t alignment) override;

    void deallocate(
      [[maybe_unused]] void* p,
      [[maybe_unused]] std::size_t bytes,
      [[maybe_unused]] std::size_t alignment) noexcept override
    {
        /* no-op. just update statistics. */

        deallocations.fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]]
    const char* name() const noexcept override
    {
        return "Bump";
    }
};

}    // namespace memory

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

#include "memory/allocator.h"

namespace memory
{

/** Bump allocator statistics. */
struct BumpAllocatorStats
{
    std::size_t used_before_reset = 0;
    std::size_t used_peak = 0;
};

/** Bump allocator. */
class BumpAllocator final
: public Allocator
{
    Allocator* allocator{nullptr};

    void* memory{nullptr};
    void* end{nullptr};
    const std::size_t alignment{alignof(std::max_align_t)};

    std::atomic<void*> base{nullptr};

    BumpAllocatorStats stats;

public:
    BumpAllocator(
      std::size_t bytes,
      Allocator* allocator)
    : allocator{allocator}
    {
        assert(bytes > 0);

        memory = allocator->allocate(
          bytes,
          alignment);
        if(memory == nullptr)
        {
            throw std::bad_alloc{};
        }

        end = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(memory) + bytes);
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
        stats.used_before_reset = size();
        stats.used_peak = std::max(stats.used_peak, stats.used_before_reset);

        base = memory;
    }

    std::size_t size() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(
                 base.load(std::memory_order::relaxed))
               - reinterpret_cast<std::uintptr_t>(memory);
    }

    std::size_t capacity() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(end)
               - reinterpret_cast<std::uintptr_t>(memory);
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
        /* no-op. */
    }

    [[nodiscard]]
    const char* name() const noexcept override
    {
        return "Bump";
    }
};

}    // namespace memory
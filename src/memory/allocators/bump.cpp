/**
 * Software Rasterizer Playground.
 *
 * Bump allocator.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "memory/allocators/bump.h"
#include "memory/utils.h"

namespace memory
{

/*
 * BumpAllocator.
 */

void* BumpAllocator::allocate(
  std::size_t bytes,
  std::size_t alignment)
{
    allocations.fetch_add(1, std::memory_order_relaxed);

    // always return a new address on each call.
    const std::size_t safe_bytes = std::max<std::size_t>(bytes, 1);

    while(true)
    {
        void* current = base.load(std::memory_order_relaxed);

        void* start = align(current, alignment);
        void* next = reinterpret_cast<void*>(
          reinterpret_cast<std::uintptr_t>(start) + safe_bytes);

        if(next > end)
        {
            // out of scratch memory
            throw std::bad_alloc{};
        }

        if(base.compare_exchange_weak(
             current,
             next,
             std::memory_order_relaxed))
        {
            return start;
        }
    }
}

}    // namespace memory
/**
 * Software Rasterizer Playground.
 *
 * Malloc allocator.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "memory/allocator.h"

namespace memory
{

/**
 * Allocator implementation backed by the system heap.
 *
 * Uses `std::malloc` for default alignments and platform-specific aligned
 * allocation functions for over-aligned allocations.
 */
struct MallocAllocator final
: public Allocator
{
    [[nodiscard]]
    void* allocate(
      std::size_t bytes,
      std::size_t alignment) override;

    void deallocate(
      void* p,
      std::size_t bytes,
      std::size_t alignment) noexcept override;

    [[nodiscard]]
    const char* name() const noexcept override
    {
        return "Malloc";
    }
};

}    // namespace memory
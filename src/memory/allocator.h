/**
 * Software Rasterizer Playground.
 *
 * Allocator base.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>

namespace memory
{

constexpr std::size_t fallback_alignment = alignof(std::max_align_t);

/** Memory domain / lifetime. */
enum class MemoryDomain
{
    Heap,
    Frame
};

/** A memory allocator. */
struct Allocator
{
    /** Virtual destructor. */
    virtual ~Allocator() = default;

    /**
     * Aligned memory allocation.
     *
     * @param bytes The byte count to allocate.
     * @param alignment The memory alignment.
     * @returns Returns aligned memory of size `bytes`.
     */
    [[nodiscard]]
    virtual void* allocate(
      std::size_t bytes,
      std::size_t alignment) = 0;

    /**
     * Aligned memory deallocation.
     *
     * @param p The memory to deallocate.
     * @param bytes The byte count to deallocate.
     * @param alignment The memory alignment.
     */
    virtual void deallocate(
      void* p,
      std::size_t bytes,
      std::size_t alignment) noexcept = 0;

    /** Return the allocator name. */
    [[nodiscard]]
    virtual const char* name() const noexcept = 0;
};

}    // namespace memory

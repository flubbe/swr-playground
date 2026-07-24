/**
 * Software Rasterizer Playground.
 *
 * Memory management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <mutex>

#include "memory/allocator.h"
#include "memory/allocators/bump.h"
#include "memory/allocators/malloc.h"

namespace memory
{

/** Memory statistics. */
struct MemoryStats
{
    /** Currently allocated byte count. */
    std::size_t bytes_live{0};

    /** Peak allocated byte count. */
    std::size_t bytes_peak{0};

    /** Total allocated bytes (non-decreasing). */
    std::size_t bytes_total_allocated{0};

    /** Allocation calls. */
    std::size_t allocate_calls{0};

    /** Deallocation calls. */
    std::size_t deallocate_calls{0};

    MemoryStats operator+(const MemoryStats& other) const
    {
        return {
          .bytes_live = bytes_live + other.bytes_live,
          .bytes_peak = bytes_peak + other.bytes_peak,
          .bytes_total_allocated = bytes_total_allocated + other.bytes_total_allocated,
          .allocate_calls = allocate_calls + other.allocate_calls,
          .deallocate_calls = deallocate_calls + other.deallocate_calls};
    }
};

/** A tracking memory allocator. */
class TrackingAllocator final
: public Allocator
{
    /** Upsteam allocator. */
    Allocator* allocator;

    std::atomic_size_t bytes_live{0};
    std::atomic_size_t bytes_peak{0};
    std::atomic_size_t bytes_total{0};

    std::atomic_size_t allocations{0};
    std::atomic_size_t deallocations{0};

    static std::atomic<uint64_t> buckets[16];
    static std::array<std::atomic<uint64_t>, 256> exact_sizes;

public:
    explicit TrackingAllocator(
      Allocator* allocator);

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
        return allocator->name();
    }

    MemoryStats stats() const;
    void print_histogram() const;
};

/** Bump memory size. */
inline constexpr std::size_t default_bump_size = 4096;    // TODO Memory to be able to grow dynamically.

class MemoryManager final
{
    MallocAllocator system_malloc_allocator;
    Allocator* global_allocator;
    BumpAllocator frame_bump_allocator;
    TrackingAllocator tracking_allocator;
    bool initialized{false};
    mutable std::mutex mutex;

    MemoryManager(
      std::size_t bump_size = default_bump_size);

public:
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    [[nodiscard]]
    static MemoryManager& instance();

    void initialize();
    void shutdown();

    [[nodiscard]]
    bool is_initialized() const;

    [[nodiscard]]
    Allocator* heap();

    [[nodiscard]]
    BumpAllocator* frame_bump();

    [[nodiscard]]
    MemoryStats stats() const;

    void print_histogram();
};

/** Initialize the memory manager. */
void initialize();

/**
 * Shut down the memory manager.
 *
 * @note Does not free any allocated memory.
 */
void shutdown();

/** Return whether the memory manager is initialized. */
[[nodiscard]]
bool is_initialized();

/** Get the global heap allocator. */
[[nodiscard]]
Allocator* heap();

/** Get the frame bump allocator. */
[[nodiscard]]
BumpAllocator* frame_bump();

/** Get tracked memory statistics. */
[[nodiscard]]
MemoryStats stats();

void print_histogram();

}    // namespace memory

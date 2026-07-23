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
#include <memory_resource>
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

/** Fast scratch allocator. */
struct ScratchAllocator
{
    /** Virtual destructor. */
    virtual ~ScratchAllocator() = default;

    /** Get the memory resource for this allocator. */
    [[nodiscard]]
    virtual std::pmr::memory_resource* resource() noexcept = 0;

    /** Reset the scratch allocations. */
    virtual void reset() = 0;
};

/** A `memory_resource` tracking allocations/deallocations. */
class TrackingMemoryResource final
: public std::pmr::memory_resource
{
    /** Associated tracking allocator. */
    TrackingAllocator allocator;

public:
    /** Deleted constructors. */
    TrackingMemoryResource() = delete;
    TrackingMemoryResource(const TrackingMemoryResource&) = delete;
    TrackingMemoryResource(TrackingMemoryResource&&) = delete;

    /**
     * Construct a tracking memory resource with an allocator.
     *
     * @param allocator The allocator to use.
     */
    explicit TrackingMemoryResource(
      Allocator* allocator)
    : allocator{allocator}
    {
        assert(allocator != nullptr);
    }

    /** Return memory statistics. */
    [[nodiscard]]
    MemoryStats stats() const
    {
        return allocator.stats();
    }

protected:
    [[nodiscard]]
    void* do_allocate(
      std::size_t bytes,
      std::size_t alignment) override
    {
        return allocator.allocate(bytes, alignment);
    }

    void do_deallocate(
      void* p,
      std::size_t bytes,
      std::size_t alignment) override
    {
        allocator.deallocate(p, bytes, alignment);
    }

    [[nodiscard]]
    bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};

/** Arena scratch allocator. */
class ScratchArena final
: public ScratchAllocator
{
    std::pmr::unsynchronized_pool_resource pool;

public:
    explicit ScratchArena(
      std::pmr::memory_resource* upstream = nullptr,
      std::pmr::pool_options options = {}) noexcept;

    [[nodiscard]]
    std::pmr::memory_resource* resource() noexcept override;

    void reset() override;
};

/** Bump memory size. */
inline constexpr std::size_t default_bump_size = 4096;    // TODO Memory to be able to grow dynamically.

class MemoryManager final
{
    MallocAllocator system_malloc_allocator;
    Allocator* global_allocator;
    BumpAllocator frame_bump_allocator;
    TrackingAllocator tracking_allocator;
    TrackingMemoryResource tracking_resource;
    ScratchArena frame_scratch_arena;
    std::pmr::memory_resource* previous_default_resource{nullptr};
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
    std::pmr::memory_resource* default_resource() noexcept;

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

/** Get the global frame scratch allocator. */
[[nodiscard]]
ScratchAllocator& frame_scratch() noexcept;

/** Get the default memory resource. */
[[nodiscard]]
std::pmr::memory_resource* default_resource() noexcept;

/** Get tracked memory statistics. */
[[nodiscard]]
MemoryStats stats();

void print_histogram();

}    // namespace memory

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
#include <cstddef>
#include <memory_resource>
#include <mutex>

namespace memory
{

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
      std::size_t alignment) = 0;

    /** Return the allocator name. */
    [[nodiscard]]
    virtual const char* name() const noexcept = 0;
};

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
      std::size_t alignment) override;

    [[nodiscard]]
    const char* name() const noexcept override
    {
        return "Malloc";
    }
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
};

/** A `memory_resource` tracking allocations/deallocations. */
class TrackingMemoryResource final
: public std::pmr::memory_resource
{
    /** Associated allocator. */
    Allocator* allocator;

    /** Currently allocated byte count. */
    std::atomic<std::size_t> bytes_live{0};

    /** Peak allocated byte count. */
    std::atomic<std::size_t> bytes_peak{0};

    /** Total allocated bytes (non-decreasing). */
    std::atomic<std::size_t> bytes_total_allocated{0};

    /** Allocation calls. */
    std::atomic<std::size_t> allocate_calls{0};

    /** Deallocation calls. */
    std::atomic<std::size_t> deallocate_calls{0};

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
      Allocator* allocator);

    /** Return memory statistics. */
    [[nodiscard]]
    MemoryStats stats() const;

protected:
    [[nodiscard]]
    void* do_allocate(
      std::size_t bytes,
      std::size_t alignment) override;

    void do_deallocate(
      void* p,
      std::size_t bytes,
      std::size_t alignment) override;

    [[nodiscard]]
    bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override;
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

class MemoryManager final
{
    MallocAllocator system_malloc_allocator;
    Allocator* global_allocator;
    TrackingMemoryResource tracking_resource;
    ScratchArena frame_scratch_arena;
    std::pmr::memory_resource* previous_default_resource{nullptr};
    bool initialized{false};
    mutable std::mutex mutex;

    MemoryManager();

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
    Allocator* get_allocator();

    [[nodiscard]]
    ScratchAllocator& frame_scratch() noexcept;

    [[nodiscard]]
    std::pmr::memory_resource* default_resource() noexcept;

    [[nodiscard]]
    MemoryStats stats() const;
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

/** Get the global allocator. */
[[nodiscard]]
Allocator* get_allocator();

/** Get the global frame scratch allocator. */
[[nodiscard]]
ScratchAllocator& frame_scratch() noexcept;

/** Get the default memory resource. */
[[nodiscard]]
std::pmr::memory_resource* default_resource() noexcept;

/** Get tracked memory statistics. */
[[nodiscard]]
MemoryStats stats();

}    // namespace memory

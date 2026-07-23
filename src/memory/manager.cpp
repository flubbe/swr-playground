/**
 * Software Rasterizer Playground.
 *
 * Memory management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <atomic>
#include <bit>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

#include <print>

#include "memory/manager.h"

#if defined(_MSC_VER)
#    include <malloc.h>
#endif

namespace memory
{

namespace
{

constexpr std::size_t fallback_alignment = alignof(std::max_align_t);
std::atomic<bool> strict_global_allocation_guards_armed{false};
std::atomic<bool> memory_manager_initialized{false};
std::atomic<Allocator*> global_allocator{nullptr};

/** Return the system allocator (malloc). */
static memory::MallocAllocator& get_system_allocator()
{
    static memory::MallocAllocator system_allocator;
    return system_allocator;
};

}    // namespace

/**
 * Helper to round up to an alignment.
 *
 * @param x The value to round up.
 * @param align Alignment, has to be a power of two.
 */
constexpr std::size_t align_up(
  std::size_t x,
  std::size_t alignment)
{
    assert(std::has_single_bit(alignment));
    return (x + alignment - 1) & ~(alignment - 1);
}

/**
 * Align a pointer.
 *
 * @param x The pointer to align.
 * @param align Alignment, has to be a power of two.
 */
template<typename T>
    requires std::is_pointer_v<T>
constexpr T align(
  T x,
  std::size_t alignment)
{
    assert(std::has_single_bit(alignment));

    auto value = reinterpret_cast<std::uintptr_t>(x);
    value = (value + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<T>(value);
}

/** Memory block header. */
struct MemoryBlockHeader
{
    /** Allocator. */
    Allocator* allocator{nullptr};

    /** Alignment used when allocating the block. */
    std::size_t alignment{0};

    /** Requested block size (i.e., excluding header and alignment). */
    std::size_t requested_size{0};

    /** Allocated block size (k.e., including header and alignment). */
    std::size_t allocated_size{0};
};

/** Type for the offset between memory block header and user data (needed for alignment). */
using offset_type = std::uint32_t;

/**
 * Get the header offset for an alignment.
 *
 * @param alignment The memory alignment.
 * @returns Returns the offset between memory block header and user data.
 */
constexpr std::size_t header_offset(
  std::size_t alignment)
{
    return align_up(
      sizeof(MemoryBlockHeader) + sizeof(offset_type),
      alignment);
}

/**
 * Return a pointer to the memory block header given a user pointer.
 *
 * @param ptr The user pointer.
 * @returns Returns a pointer to the memory block header.
 */
MemoryBlockHeader* header_from_user(
  void* ptr)
{
    auto* user = static_cast<std::byte*>(ptr);

    offset_type offset;
    std::memcpy(&offset,
                user - sizeof(offset),
                sizeof(offset));

    return reinterpret_cast<MemoryBlockHeader*>(user - offset);
}

/*
 * MallocAllocator.
 */

void* MallocAllocator::allocate(
  std::size_t bytes,
  std::size_t alignment)
{
    const std::size_t safe_bytes = std::max<std::size_t>(bytes, 1);
    const std::size_t safe_alignment = std::max<std::size_t>(alignment, alignof(void*));

    if(safe_alignment <= fallback_alignment)
    {
        if(void* ptr = std::malloc(safe_bytes);
           ptr != nullptr)
        {
            return ptr;
        }
        throw std::bad_alloc{};
    }

#if defined(_MSC_VER)
    if(void* ptr = _aligned_malloc(safe_bytes, safe_alignment);
       ptr != nullptr)
    {
        return ptr;
    }
    throw std::bad_alloc{};
#else
    // NOTE Memory allocated with posix_memalign is freed via std::free.

    void* ptr = nullptr;
    if(posix_memalign(&ptr, safe_alignment, safe_bytes) == 0
       && ptr != nullptr)
    {
        return ptr;
    }
    throw std::bad_alloc{};
#endif
}

void MallocAllocator::deallocate(
  void* p,
  [[maybe_unused]] std::size_t bytes,
  [[maybe_unused]] std::size_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}

/*
 * BumpAllocator.
 */

void* BumpAllocator::allocate(
  std::size_t bytes,
  std::size_t alignment)
{
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

/*
 * TrackingAllocator.
 */

std::atomic<uint64_t> TrackingAllocator::buckets[16] = {};
std::array<std::atomic<uint64_t>, 256> TrackingAllocator::exact_sizes = {};

TrackingAllocator::TrackingAllocator(
  Allocator* allocator)
: allocator{allocator}
{
    assert(allocator != nullptr);
}

void* TrackingAllocator::allocate(
  std::size_t bytes,
  std::size_t alignment)
{
    void* allocation = allocator->allocate(bytes, alignment);

    allocations.fetch_add(1, std::memory_order_relaxed);
    bytes_total.fetch_add(bytes, std::memory_order_relaxed);

    const std::size_t live_after =
      bytes_live.fetch_add(bytes, std::memory_order_relaxed) + bytes;

    std::size_t previous_peak = bytes_peak.load(std::memory_order_relaxed);
    while(live_after > previous_peak
          && !bytes_peak.compare_exchange_weak(
            previous_peak,
            live_after,
            std::memory_order_relaxed,
            std::memory_order_relaxed))
    {
    }

    auto b = std::min<size_t>(15, std::bit_width(bytes));
    ++buckets[b];

    if(bytes < exact_sizes.size())
        ++exact_sizes[bytes];

    return allocation;
}

void TrackingAllocator::deallocate(
  void* p,
  std::size_t bytes,
  std::size_t alignment) noexcept
{
    deallocations.fetch_add(1, std::memory_order_relaxed);
    bytes_live.fetch_sub(bytes, std::memory_order_relaxed);
    allocator->deallocate(p, bytes, alignment);
}

MemoryStats TrackingAllocator::stats() const
{
    return MemoryStats{
      .bytes_live = bytes_live.load(std::memory_order_relaxed),
      .bytes_peak = bytes_peak.load(std::memory_order_relaxed),
      .bytes_total_allocated = bytes_total.load(std::memory_order_relaxed),
      .allocate_calls = allocations.load(std::memory_order_relaxed),
      .deallocate_calls = deallocations.load(std::memory_order_relaxed),
    };
}

void TrackingAllocator::print_histogram() const
{
    std::println("Allocation histogram:");
    for(std::size_t i = 0; i < 16; ++i)
    {
        std::println("   {}: {}", i, buckets[i].load(std::memory_order::relaxed));
    }
    std::println("Exact sizes:");
    for(std::size_t i = 0; i < 256; ++i)
    {
        auto s = exact_sizes[i].load(std::memory_order::relaxed);
        if(s != 0)
        {
            std::println("    {}: {}", i, s);
        }
    }
}

/*
 * ScratchArena.
 */

ScratchArena::ScratchArena(
  std::pmr::memory_resource* upstream,
  std::pmr::pool_options options) noexcept
: pool{
    options,
    upstream != nullptr
      ? upstream
      : memory::default_resource()}
{
}

std::pmr::memory_resource* ScratchArena::resource() noexcept
{
    return &pool;
}

void ScratchArena::reset()
{
    pool.release();
}

/*
 * MemoryManager.
 */

MemoryManager::MemoryManager(
  std::size_t bump_size)
: system_malloc_allocator{}
, global_allocator{&system_malloc_allocator}
, frame_bump_allocator{bump_size, global_allocator}
, tracking_allocator{global_allocator}
, tracking_resource{global_allocator}
, frame_scratch_arena{&tracking_resource}
{
    ::memory::global_allocator.store(
      &tracking_allocator,
      std::memory_order_release);
}

MemoryManager& MemoryManager::instance()
{
    static MemoryManager manager{};
    return manager;
}

void MemoryManager::initialize()
{
    std::scoped_lock lock{mutex};
    if(initialized)
    {
        return;
    }

    previous_default_resource = std::pmr::get_default_resource();
    std::pmr::set_default_resource(&tracking_resource);
    initialized = true;
    memory_manager_initialized.store(true, std::memory_order_release);
    strict_global_allocation_guards_armed.store(true, std::memory_order_release);
}

void MemoryManager::shutdown()
{
    std::scoped_lock lock{mutex};
    if(!initialized)
    {
        return;
    }

    strict_global_allocation_guards_armed.store(false, std::memory_order_release);
    memory_manager_initialized.store(false, std::memory_order_release);

    std::pmr::set_default_resource(
      previous_default_resource != nullptr
        ? previous_default_resource
        : std::pmr::new_delete_resource());

    previous_default_resource = nullptr;
    initialized = false;
}

bool MemoryManager::is_initialized() const
{
    return initialized;
}

Allocator* MemoryManager::heap()
{
    return &tracking_allocator;
}

BumpAllocator* MemoryManager::frame_bump()
{
    return &frame_bump_allocator;
}

std::pmr::memory_resource* MemoryManager::default_resource() noexcept
{
    return &tracking_resource;
}

MemoryStats MemoryManager::stats() const
{
    return tracking_allocator.stats() + tracking_resource.stats();
}

void MemoryManager::print_histogram()
{
    tracking_allocator.print_histogram();
}

/*
 * Memory manager singleton and interface.
 */

void initialize()
{
    MemoryManager::instance().initialize();
}

void shutdown()
{
    MemoryManager::instance().shutdown();
}

bool is_initialized()
{
    return MemoryManager::instance().is_initialized();
}

Allocator* heap()
{
    return MemoryManager::instance().heap();
}

BumpAllocator* frame_bump()
{
    return MemoryManager::instance().frame_bump();
}

std::pmr::memory_resource* default_resource() noexcept
{
    return MemoryManager::instance().default_resource();
}

MemoryStats stats()
{
    return MemoryManager::instance().stats();
}

void print_histogram()
{
    MemoryManager::instance().print_histogram();
}

}    // namespace memory

/*
 * Global operator new/delete.
 */

void* operator new(
  std::size_t bytes)
{
    return memory::get_system_allocator()
      .allocate(
        bytes,
        alignof(std::max_align_t));
}

void* operator new[](
  std::size_t bytes)
{
    return memory::get_system_allocator()
      .allocate(
        bytes,
        alignof(std::max_align_t));
}

void* operator new(
  std::size_t bytes,
  std::align_val_t alignment)
{
    return memory::get_system_allocator()
      .allocate(
        bytes,
        std::to_underlying(alignment));
}

void* operator new[](
  std::size_t bytes,
  std::align_val_t alignment)
{
    return memory::get_system_allocator()
      .allocate(
        bytes,
        std::to_underlying(alignment));
}

void* operator new(
  std::size_t bytes,
  const std::nothrow_t&) noexcept
{
    try
    {
        return ::operator new(bytes);
    }
    catch(...)
    {
        return nullptr;
    }
}

void* operator new[](
  std::size_t bytes,
  const std::nothrow_t&) noexcept
{
    try
    {
        return ::operator new[](bytes);
    }
    catch(...)
    {
        return nullptr;
    }
}

void* operator new(
  std::size_t bytes,
  std::align_val_t alignment,
  const std::nothrow_t&) noexcept
{
    try
    {
        return ::operator new(bytes, alignment);
    }
    catch(...)
    {
        return nullptr;
    }
}

void* operator new[](
  std::size_t bytes,
  std::align_val_t alignment,
  const std::nothrow_t&) noexcept
{
    try
    {
        return ::operator new[](bytes, alignment);
    }
    catch(...)
    {
        return nullptr;
    }
}

void operator delete(void* p) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    memory::get_system_allocator()
      .deallocate(p, 0, 0);
}

void operator delete[](void* p) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    memory::get_system_allocator()
      .deallocate(p, 0, 0);
}

void operator delete(
  void* p,
  std::size_t bytes) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    memory::get_system_allocator()
      .deallocate(p, bytes, 0);
}

void operator delete[](
  void* p,
  std::size_t bytes) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    memory::get_system_allocator()
      .deallocate(p, bytes, 0);
}

void operator delete(
  void* p,
  std::align_val_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    memory::get_system_allocator()
      .deallocate(p, 0, std::to_underlying(alignment));
}

void operator delete[](
  void* p,
  std::align_val_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    memory::get_system_allocator()
      .deallocate(p, 0, std::to_underlying(alignment));
}

void operator delete(
  void* p,
  std::size_t bytes,
  std::align_val_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    memory::get_system_allocator()
      .deallocate(p, bytes, std::to_underlying(alignment));
}

void operator delete[](
  void* p,
  std::size_t bytes,
  std::align_val_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    memory::get_system_allocator()
      .deallocate(p, bytes, std::to_underlying(alignment));
}

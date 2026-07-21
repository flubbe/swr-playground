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

#include "memory_manager.h"

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

/** Return the bootstrap allocator (malloc). */
static memory::MallocAllocator& get_bootstrap_allocator()
{
    static memory::MallocAllocator allocator;
    return allocator;
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

/** Memory block header. */
struct MemoryBlockHeader
{
    /** Allocator. */
    Allocator* allocator{nullptr};

    /** Alignment used when allocating the block. */
    std::uint32_t alignment{0};

    /** Requested block size (i.e., excluding header and alignment). */
    std::size_t size{0};
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
  std::size_t,
  std::size_t alignment)
{
    (void)alignment;

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
 * TrackingMemoryResource.
 */

TrackingMemoryResource::TrackingMemoryResource(
  Allocator* allocator)
: allocator{allocator}
{
}

MemoryStats TrackingMemoryResource::stats() const
{
    return MemoryStats{
      .bytes_live = bytes_live.load(std::memory_order_relaxed),
      .bytes_peak = bytes_peak.load(std::memory_order_relaxed),
      .bytes_total_allocated = bytes_total_allocated.load(std::memory_order_relaxed),
      .allocate_calls = allocate_calls.load(std::memory_order_relaxed),
      .deallocate_calls = deallocate_calls.load(std::memory_order_relaxed),
    };
}

void* TrackingMemoryResource::do_allocate(
  std::size_t bytes,
  std::size_t alignment)
{
    void* allocation = allocator->allocate(bytes, alignment);

    allocate_calls.fetch_add(1, std::memory_order_relaxed);
    bytes_total_allocated.fetch_add(bytes, std::memory_order_relaxed);

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

    return allocation;
}

void TrackingMemoryResource::do_deallocate(
  void* p,
  std::size_t bytes,
  std::size_t alignment)
{
    deallocate_calls.fetch_add(1, std::memory_order_relaxed);
    bytes_live.fetch_sub(bytes, std::memory_order_relaxed);
    allocator->deallocate(p, bytes, alignment);
}

bool TrackingMemoryResource::do_is_equal(
  const std::pmr::memory_resource& other) const noexcept
{
    return this == &other;
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

MemoryManager::MemoryManager()
: system_malloc_allocator{}
, global_allocator{&system_malloc_allocator}
, tracking_resource{global_allocator}
, frame_scratch_arena{&tracking_resource}
{
    ::memory::global_allocator.store(
      global_allocator,
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
    std::scoped_lock lock{mutex};
    return initialized;
}

Allocator* MemoryManager::get_allocator()
{
    std::scoped_lock lock{mutex};
    return global_allocator;
}

ScratchAllocator& MemoryManager::frame_scratch() noexcept
{
    return frame_scratch_arena;
}

std::pmr::memory_resource* MemoryManager::default_resource() noexcept
{
    return &tracking_resource;
}

MemoryStats MemoryManager::stats() const
{
    return tracking_resource.stats();
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

Allocator* get_allocator()
{
    return MemoryManager::instance().get_allocator();
}

ScratchAllocator& frame_scratch() noexcept
{
    return MemoryManager::instance().frame_scratch();
}

std::pmr::memory_resource* default_resource() noexcept
{
    return MemoryManager::instance().default_resource();
}

MemoryStats stats()
{
    return MemoryManager::instance().stats();
}

}    // namespace memory

/*
 * Global operator new/delete.
 */

namespace
{

[[noreturn]]
void fail_memory_manager_unavailable()
{
    // NOTE Uses fputs since it might be called during static initialization.
    std::fputs(
      "FATAL: global operator new/delete called before memory manager initialization.\n",
      stderr);
    std::abort();
}

memory::Allocator* checked_allocator()
{
    // NOTE Bootstrapping might be needed for tests.
#if defined(SWR_MEMORY_ALLOCATOR_ALLOW_BOOTSTRAP)
    if(!memory::memory_manager_initialized.load(std::memory_order_acquire))
    {
        return &memory::get_bootstrap_allocator();
    }
#else
    if(!memory::memory_manager_initialized.load(std::memory_order_acquire))
    {
        if(memory::strict_global_allocation_guards_armed.load(std::memory_order_acquire))
        {
            fail_memory_manager_unavailable();
        }

        return &memory::get_bootstrap_allocator();
    }
#endif

    if(memory::Allocator* allocator = memory::global_allocator.load(std::memory_order_acquire);
       allocator != nullptr)
    {
        return allocator;
    }

    fail_memory_manager_unavailable();
}

}    // namespace

void* operator new(
  std::size_t bytes)
{
    auto* allocator = checked_allocator();

    constexpr std::size_t alignment = alignof(std::max_align_t);
    auto offset = memory::header_offset(alignment);

    auto* raw = static_cast<std::byte*>(
      allocator->allocate(offset + bytes, alignment));

    std::construct_at(
      reinterpret_cast<memory::MemoryBlockHeader*>(raw),
      memory::MemoryBlockHeader{
        .allocator = allocator,
        .alignment = alignment,
        .size = bytes});

    auto* user = raw + offset;

    auto stored_offset = static_cast<memory::offset_type>(offset);
    std::memcpy(user - sizeof(stored_offset),
                &stored_offset,
                sizeof(stored_offset));

    return user;
}

void* operator new[](
  std::size_t bytes)
{
    auto* allocator = checked_allocator();

    constexpr std::size_t alignment = alignof(std::max_align_t);
    auto offset = memory::header_offset(alignment);

    auto* raw = static_cast<std::byte*>(
      allocator->allocate(offset + bytes, alignment));

    std::construct_at(
      reinterpret_cast<memory::MemoryBlockHeader*>(raw),
      memory::MemoryBlockHeader{
        .allocator = allocator,
        .alignment = alignment,
        .size = bytes});

    auto* user = raw + offset;

    auto stored_offset = static_cast<memory::offset_type>(offset);
    std::memcpy(user - sizeof(stored_offset),
                &stored_offset,
                sizeof(stored_offset));

    return user;
}

void* operator new(
  std::size_t bytes,
  std::align_val_t alignment)
{
    auto* allocator = checked_allocator();

    auto align = static_cast<std::size_t>(alignment);
    auto offset = memory::header_offset(align);

    auto* raw = static_cast<std::byte*>(
      allocator->allocate(offset + bytes, align));

    std::construct_at(
      reinterpret_cast<memory::MemoryBlockHeader*>(raw),
      memory::MemoryBlockHeader{
        .allocator = allocator,
        .alignment = static_cast<std::uint32_t>(alignment),
        .size = bytes});

    auto* user = raw + offset;

    auto stored_offset = static_cast<memory::offset_type>(offset);
    std::memcpy(user - sizeof(stored_offset),
                &stored_offset,
                sizeof(stored_offset));

    return user;
}

void* operator new[](
  std::size_t bytes,
  std::align_val_t alignment)
{
    auto* allocator = checked_allocator();

    auto align = static_cast<std::size_t>(alignment);
    auto offset = memory::header_offset(align);

    auto* raw = static_cast<std::byte*>(
      allocator->allocate(offset + bytes, align));

    std::construct_at(
      reinterpret_cast<memory::MemoryBlockHeader*>(raw),
      memory::MemoryBlockHeader{
        .allocator = allocator,
        .alignment = static_cast<std::uint32_t>(alignment),
        .size = bytes});

    auto* user = raw + offset;

    auto stored_offset = static_cast<memory::offset_type>(offset);
    std::memcpy(user - sizeof(stored_offset),
                &stored_offset,
                sizeof(stored_offset));

    return user;
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

    auto* header = memory::header_from_user(p);
    assert(header->allocator != nullptr);

    header->allocator->deallocate(
      header,
      header->size,
      header->alignment);
}

void operator delete[](void* p) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    auto* header = memory::header_from_user(p);
    assert(header->allocator != nullptr);

    header->allocator->deallocate(
      header,
      header->size,
      header->alignment);
}

void operator delete(
  void* p,
  [[maybe_unused]] std::size_t bytes) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    auto* header = memory::header_from_user(p);
    assert(header->allocator != nullptr);
    assert(header->size == bytes);

    header->allocator->deallocate(
      header,
      header->size,
      header->alignment);
}

void operator delete[](
  void* p,
  [[maybe_unused]] std::size_t bytes) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    auto* header = memory::header_from_user(p);
    assert(header->allocator != nullptr);
    assert(header->size == bytes);

    header->allocator->deallocate(
      header,
      header->size,
      header->alignment);
}

void operator delete(
  void* p,
  [[maybe_unused]] std::align_val_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    auto* header = memory::header_from_user(p);
    assert(header->allocator != nullptr);
    assert(header->alignment == static_cast<std::uint32_t>(alignment));

    header->allocator->deallocate(
      header,
      header->size,
      header->alignment);
}

void operator delete[](
  void* p,
  [[maybe_unused]] std::align_val_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    auto* header = memory::header_from_user(p);
    assert(header->allocator != nullptr);
    assert(header->alignment == static_cast<std::uint32_t>(alignment));

    header->allocator->deallocate(
      header,
      header->size,
      header->alignment);
}

void operator delete(
  void* p,
  [[maybe_unused]] std::size_t bytes,
  [[maybe_unused]] std::align_val_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    auto* header = memory::header_from_user(p);
    assert(header->allocator != nullptr);
    assert(header->alignment == static_cast<std::uint32_t>(alignment));
    assert(header->size == bytes);

    header->allocator->deallocate(
      header,
      header->size,
      header->alignment);
}

void operator delete[](
  void* p,
  [[maybe_unused]] std::size_t bytes,
  [[maybe_unused]] std::align_val_t alignment) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    auto* header = memory::header_from_user(p);
    assert(header->allocator != nullptr);
    assert(header->alignment == static_cast<std::uint32_t>(alignment));
    assert(header->size == bytes);

    header->allocator->deallocate(
      header,
      header->size,
      header->alignment);
}

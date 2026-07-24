/**
 * Software Rasterizer Playground.
 *
 * Global new/delete overrides.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#if SWR_OVERRIDE_GLOBAL_NEW

#    include <utility>

#    include "memory/manager.h"

namespace
{

/** Return the system allocator (malloc). */
memory::MallocAllocator& get_system_allocator()
{
    static memory::MallocAllocator system_allocator;
    return system_allocator;
};

}    // namespace

/*
 * Global operator new/delete.
 */

void* operator new(
  std::size_t bytes)
{
    return get_system_allocator()
      .allocate(
        bytes,
        alignof(std::max_align_t));
}

void* operator new[](
  std::size_t bytes)
{
    return get_system_allocator()
      .allocate(
        bytes,
        alignof(std::max_align_t));
}

void* operator new(
  std::size_t bytes,
  std::align_val_t alignment)
{
    return get_system_allocator()
      .allocate(
        bytes,
        std::to_underlying(alignment));
}

void* operator new[](
  std::size_t bytes,
  std::align_val_t alignment)
{
    return get_system_allocator()
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

    get_system_allocator()
      .deallocate(p, 0, 0);
}

void operator delete[](void* p) noexcept
{
    if(p == nullptr)
    {
        return;
    }

    get_system_allocator()
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

    get_system_allocator()
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

    get_system_allocator()
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

    get_system_allocator()
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

    get_system_allocator()
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

    get_system_allocator()
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

    get_system_allocator()
      .deallocate(p, bytes, std::to_underlying(alignment));
}

#endif /* SWR_OVERRIDE_GLOBAL_NEW */
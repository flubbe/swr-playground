/**
 * Software Rasterizer Playground.
 *
 * dynamic memory management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <concepts>
#include <memory>
#include <utility>

#include "memory/manager.h"

namespace swr
{

#if SWR_USE_CUSTOM_STD_ALLOCATORS

template<typename T>
struct DefaultDeleter
{
    DefaultDeleter() = default;

    template<typename U>
        requires std::convertible_to<U*, T*>
    DefaultDeleter(const DefaultDeleter<U>&) noexcept
    {
    }

    void operator()(T* p) const
    {
        if(p == nullptr)
        {
            return;
        }

        p->~T();
        memory::heap()->deallocate(
          p,
          sizeof(T),
          alignof(T));
    }
};

template<
  typename T,
  typename... Args>
auto make_unique(
  Args&&... args)
{
    constexpr std::size_t size = sizeof(T);
    constexpr std::size_t alignment = alignof(T);

    void* mem = memory::heap()->allocate(
      size,
      alignment);

    try
    {
        T* p = new(mem) T(std::forward<Args>(args)...);
        return std::unique_ptr<T, DefaultDeleter<T>>(p);
    }
    catch(...)
    {
        memory::heap()->deallocate(
          mem,
          size,
          alignment);
        throw;
    }
}

template<
  typename T,
  typename Deleter = DefaultDeleter<T>>
using unique_ptr = std::unique_ptr<T, Deleter>;

template<typename T>
using shared_ptr = std::shared_ptr<T>;

#else /* SWR_USE_CUSTOM_STD_ALLOCATORS */

template<
  typename T,
  typename... Args>
auto make_unique(
  Args&&... args)
{
    return std::make_unique<T>(
      std::forward<Args>(args)...);
}

template<
  typename T,
  typename Deleter = std::default_delete<T>>
using unique_ptr = std::unique_ptr<T, Deleter>;

template<typename T>
using shared_ptr = std::shared_ptr<T>;

#endif

}    // namespace swr

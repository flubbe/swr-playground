/**
 * Software Rasterizer Playground.
 *
 * Dynamic memory management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <concepts>
#include <memory>
#include <stdexcept>
#include <utility>

#include "memory/manager.h"

namespace swr
{

#if SWR_USE_CUSTOM_STD_ALLOCATORS

template<typename T>
struct DefaultDeleter
{
    using DeallocateFn = void (*)(void*) noexcept;
    DeallocateFn deallocate{default_deallocate};

    static void default_deallocate(void* p) noexcept
    {
        memory::heap().deallocate(
          p,
          sizeof(T),
          alignof(T),
          memory::MemoryTag::UniquePtr);
    }

    constexpr DefaultDeleter() noexcept = default;

    constexpr explicit DefaultDeleter(
      DeallocateFn fn)
    : deallocate(fn)
    {
        if(deallocate == nullptr)
        {
            throw std::invalid_argument{
              "Deleter requires a deletion function"};
        }
    }

    template<typename U>
        requires std::convertible_to<U*, T*>
    constexpr DefaultDeleter(
      const DefaultDeleter<U>& other)
    : deallocate(other.deallocate)
    {
        if(deallocate == nullptr)
        {
            throw std::invalid_argument{
              "Cannot convert a Deleter with a null deletion function"};
        }
    }

    void operator()(T* p) const noexcept
    {
        if(p == nullptr)
        {
            return;
        }

        p->~T();

        deallocate(p);
    }
};

template<typename T>
using unique_ptr = std::unique_ptr<T, DefaultDeleter<T>>;

template<
  typename T,
  typename... Args>
auto make_unique(Args&&... args)
{
    constexpr std::size_t Size = sizeof(T);
    constexpr std::size_t Alignment = alignof(T);

    void* mem = memory::heap().allocate(
      Size,
      Alignment,
      memory::MemoryTag::UniquePtr);

    auto deallocate_fn = [](void* p) noexcept
    {
        memory::heap().deallocate(
          p,
          Size,
          Alignment,
          memory::MemoryTag::UniquePtr);
    };

    using Deleter = DefaultDeleter<T>;

    try
    {
        return std::unique_ptr<T, Deleter>{
          new(mem) T{std::forward<Args>(args)...},
          Deleter{deallocate_fn}};
    }
    catch(...)
    {
        memory::heap().deallocate(
          mem,
          Size,
          Alignment,
          memory::MemoryTag::UniquePtr);
        throw;
    }
}

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

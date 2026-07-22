/**
 * Software Rasterizer Playground.
 *
 * std allocator adapter.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "memory/manager.h"

namespace swr
{

enum class MemoryTag
{
    Unknown,
    Deque,
    List,
    String,
    UnorderedMap,
    Vector,
};

template<
  typename T,
  MemoryTag Tag = MemoryTag::Unknown>
struct StdAllocator
{
    static constexpr MemoryTag tag = Tag;

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    template<
      typename U>
    struct rebind
    {
        using other = StdAllocator<U, Tag>;
    };

    StdAllocator() noexcept = default;

    template<
      typename U,
      MemoryTag OtherTag>
    StdAllocator(
      const StdAllocator<U, OtherTag>&) noexcept
    {
    }

    T* allocate(
      std::size_t n)
    {
        return static_cast<T*>(
          memory::get_allocator()->allocate(
            n * sizeof(T),
            alignof(T)));
    }

    void deallocate(
      T* p,
      std::size_t n)
    {
        memory::get_allocator()->deallocate(
          p,
          n * sizeof(T),
          alignof(T));
    }
};

}    // namespace swr

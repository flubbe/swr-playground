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

#include "containers/type_traits.h"
#include "memory/manager.h"

namespace swr
{

/** Memory tags (container types). */
enum class MemoryTag
{
    Unknown,
    Deque,
    String,
    UnorderedSet,
    UnorderedMap,
    Vector,
};

template<
  typename T,
  MemoryTag Tag = MemoryTag::Unknown,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
struct StdAllocator
{
    static constexpr MemoryTag tag = Tag;
    static constexpr memory::MemoryDomain domain = Domain;

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    template<
      typename U>
    struct rebind
    {
        using other = StdAllocator<U, Tag, Domain>;
    };

    StdAllocator() noexcept = default;

    template<
      typename U,
      MemoryTag OtherTag,
      memory::MemoryDomain OtherDomain>
    StdAllocator(
      const StdAllocator<U, OtherTag, OtherDomain>&) noexcept
    {
    }

    T* allocate(
      std::size_t n)
    {
        if constexpr(Domain == memory::MemoryDomain::Heap)
        {
            return static_cast<T*>(
              memory::heap()->allocate(
                n * sizeof(T),
                alignof(T)));
        }
        else if constexpr(Domain == memory::MemoryDomain::Frame)
        {
            return static_cast<T*>(
              memory::frame_bump()->allocate(
                n * sizeof(T),
                alignof(T)));
        }
        else
        {
            static_assert(
              swr::false_type_v<
                std::integral_constant<
                  memory::MemoryDomain,
                  Domain>>,
              "Memory domain not supported by this allocator");
        }
    }

    void deallocate(
      T* p,
      std::size_t n)
    {
        if constexpr(Domain == memory::MemoryDomain::Heap)
        {
            memory::heap()->deallocate(
              p,
              n * sizeof(T),
              alignof(T));
        }
        else if constexpr(Domain == memory::MemoryDomain::Frame)
        {
            memory::frame_bump()->deallocate(
              p,
              n * sizeof(T),
              alignof(T));
        }
        else
        {
            static_assert(
              swr::false_type_v<
                std::integral_constant<
                  memory::MemoryDomain,
                  Domain>>,
              "Memory domain not supported by this allocator");
        }
    }

    friend constexpr bool operator==(
      const StdAllocator&,
      const StdAllocator&) = default;
};

}    // namespace swr

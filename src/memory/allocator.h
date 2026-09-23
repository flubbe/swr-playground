/**
 * Software Rasterizer Playground.
 *
 * Allocator base.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

namespace memory
{

/** Memory tags (e.g. container types). */
enum class MemoryTag : std::uint8_t
{
    None,         /** No tag. */
    New,          /** operator new.  */
    Deque,        /** swr::deque. */
    String,       /** swr::string. */
    UnorderedSet, /** swr::unordered_set. */
    UnorderedMap, /** swr::unordered_map. */
    Vector,       /** swr::vector. */
    UniquePtr,    /** swr::unique_ptr. */
    Page,         /** Arena page. */
    Bump,         /** Bump. */
    Object,       /** Object hierarchy. */

    Count /** Tag count. Not a tag. */
};

/** Return the memory tag as a readable string. */
[[nodiscard]]
constexpr const char* to_string(
  MemoryTag tag)
{
    switch(tag)
    {
    case MemoryTag::None:
        return "None";
    case MemoryTag::New:
        return "New";
    case MemoryTag::Deque:
        return "Deque";
    case MemoryTag::String:
        return "String";
    case MemoryTag::UnorderedSet:
        return "UnorderedSet";
    case MemoryTag::UnorderedMap:
        return "UnorderedMap";
    case MemoryTag::Vector:
        return "Vector";
    case MemoryTag::UniquePtr:
        return "UniquePtr";
    case MemoryTag::Page:
        return "Page";
    case MemoryTag::Bump:
        return "Bump";
    case MemoryTag::Object:
        return "Object";
    case MemoryTag::Count:
        /* Fall through to unreachable. */
        break;
    }

    std::unreachable();
}

constexpr std::size_t fallback_alignment = alignof(std::max_align_t);

/** Memory domain / lifetime. */
enum class MemoryDomain
{
    Heap,
    Frame
};

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
     * @param tag Memory tag.
     * @returns Returns aligned memory of size `bytes`.
     */
    [[nodiscard]]
    virtual void* allocate(
      std::size_t bytes,
      std::size_t alignment,
      MemoryTag tag) = 0;

    /**
     * Aligned memory deallocation.
     *
     * @param p The memory to deallocate.
     * @param bytes The byte count to deallocate.
     * @param alignment The memory alignment.
     * @param tag Memory tag.
     */
    virtual void deallocate(
      void* p,
      std::size_t bytes,
      std::size_t alignment,
      MemoryTag tag) noexcept = 0;

    /** Return the allocator name. */
    [[nodiscard]]
    virtual const char* name() const noexcept = 0;
};

}    // namespace memory

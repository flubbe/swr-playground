/**
 * Software Rasterizer Playground.
 *
 * Arena allocator.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "memory/allocators/arena.h"
#include "memory/utils.h"

namespace memory
{

void* ArenaAllocator::allocate(
  size_t bytes,
  size_t alignment)
{
    ++allocations;

    bytes = std::max(bytes, size_t{1});

    ArenaPage* page = head;
    for(; page != nullptr; page = page->next)
    {
        auto* start = align(page->data + page->used, alignment);
        auto* next = start + bytes;

        if(next <= page->data + page->capacity)
        {
            page->used = next - page->data;
            total_size += bytes;

            return start;
        }
    }

    page = new_page(bytes, alignment);
    assert(page != nullptr);

    auto* start = align(page->data + page->used, alignment);
    auto* next = start + bytes;
    assert(next <= page->data + page->capacity);

    page->used = next - page->data;
    total_size += bytes;

    return start;
}

}    // namespace memory

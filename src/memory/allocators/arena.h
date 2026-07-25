/**
 * Software Rasterizer Playground.
 *
 * Arena allocator. Reallocates when blocks are full.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <new>
#include <stdexcept>

#include "memory/allocator.h"
#include "memory/utils.h"

namespace memory
{

/** Arena allocator statistics. */
struct ArenaAllocatorStats
{
    std::size_t pages = 0;
    std::size_t allocations = 0;
    std::size_t deallocations = 0;
    std::size_t used_before_reset = 0;
    std::size_t used_peak = 0;
    std::size_t free_before_reset = 0;
};

/** Arena page. */
struct ArenaPage
{
    ArenaPage* next{nullptr};

    std::size_t page_size{0};
    std::size_t capacity{0};
    std::size_t used{0};
    std::byte* data{nullptr};
};

/** Arena allocator. */
class ArenaAllocator final
: public Allocator
{
    const std::size_t alignment{alignof(std::max_align_t)};

    Allocator* allocator{nullptr};

    std::size_t default_page_size{64 * 1024};
    ArenaPage* head{nullptr};

    std::size_t total_size{0};
    std::size_t total_capacity{0};

    std::size_t allocations{0};
    std::size_t deallocations{0};
    std::size_t pages{0};
    ArenaAllocatorStats stats;

    ArenaPage* new_page(
      std::size_t size,
      std::size_t data_alignment)
    {
        constexpr std::size_t header = sizeof(ArenaPage);
        const std::size_t padding = data_alignment - 1;

        std::size_t page_size =
          std::max(default_page_size,
                   header + padding + size);

        void* memory = allocator->allocate(
          page_size,
          alignment);
        if(memory == nullptr)
        {
            throw std::bad_alloc{};
        }

        auto* page = std::construct_at(
          reinterpret_cast<ArenaPage*>(memory));

        auto* data = align(
          reinterpret_cast<std::byte*>(page + 1),
          data_alignment);

        page->next = head;
        page->page_size = page_size;
        page->capacity = page_size - (data - reinterpret_cast<std::byte*>(page));
        page->used = 0;
        page->data = data;

        head = page;
        ++pages;

        total_capacity += page->capacity;

        return page;
    }

public:
    ArenaAllocator(
      Allocator* allocator,
      std::size_t default_page_size = 64 * 1024)
    : allocator{allocator}
    , default_page_size{default_page_size}
    {
        if(allocator == nullptr)
        {
            throw std::invalid_argument{
              "allocator must not be null"};
        }
    }
    ~ArenaAllocator()
    {
        while(head)
        {
            auto* next = head->next;

            allocator->deallocate(
              head,
              head->page_size,
              alignment);

            head = next;
        }
    }

    void reset() noexcept
    {
        stats.pages = pages;
        stats.allocations = allocations;
        stats.deallocations = deallocations;
        stats.used_before_reset = size();
        stats.used_peak = std::max(stats.used_peak, stats.used_before_reset);
        stats.free_before_reset = capacity() - stats.used_before_reset;

        for(auto* p = head; p != nullptr; p = p->next)
        {
            p->used = 0;
        }

        total_size = 0;
        allocations = 0;
        deallocations = 0;
    }

    std::size_t size() const noexcept
    {
        return total_size;
    }

    std::size_t capacity() const noexcept
    {
        return total_capacity;
    }

    ArenaAllocatorStats get_stats() const noexcept
    {
        return stats;
    }

    [[nodiscard]]
    void* allocate(
      std::size_t bytes,
      std::size_t alignment) override;

    void deallocate(
      [[maybe_unused]] void* p,
      [[maybe_unused]] std::size_t bytes,
      [[maybe_unused]] std::size_t alignment) noexcept override
    {
        /* no-op. just update statistics. */

        ++deallocations;
    }

    [[nodiscard]]
    const char* name() const noexcept override
    {
        return "Arena";
    }
};

}    // namespace memory

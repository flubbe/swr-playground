/**
 * Software Rasterizer Playground.
 *
 * Malloc allocator.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cstdlib>

#include "memory/allocators/malloc.h"

namespace memory
{

/*
 * MallocAllocator.
 */

void* MallocAllocator::allocate(
  std::size_t bytes,
  std::size_t alignment)
{
    const std::size_t safe_bytes = std::max<std::size_t>(bytes, 1);

    if(alignment <= fallback_alignment)
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

    if(void* ptr = nullptr;
       posix_memalign(&ptr, alignment, safe_bytes) == 0
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

}    // namespace memory
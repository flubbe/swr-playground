/**
 * Software Rasterizer Playground.
 *
 * std deque adapter.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <deque>

#include "containers/allocator.h"

namespace swr
{

#if SWR_USE_CUSTOM_STD_ALLOCATORS

template<
  typename T,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
using deque = std::deque<
  T,
  swr::StdAllocator<
    T,
    MemoryTag::Deque,
    Domain>>;

#else /* SWR_USE_CUSTOM_STD_ALLOCATORS */

template<
  typename T,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
using deque = std::deque<T, std::allocator<T>>;

#endif /* SWR_USE_CUSTOM_STD_ALLOCATORS */

}    // namespace swr

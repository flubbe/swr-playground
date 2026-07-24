/**
 * Software Rasterizer Playground.
 *
 * std vector adapter.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <vector>

#include "containers/allocator.h"

namespace swr
{

#if SWR_USE_CUSTOM_STD_ALLOCATORS

template<
  typename T,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
using vector = std::vector<
  T,
  swr::StdAllocator<
    T,
    MemoryTag::Vector,
    Domain>>;

#else /* SWR_USE_CUSTOM_STD_ALLOCATORS */

template<
  typename T,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
using vector = std::vector<
  T,
  std::allocator<T>>;

#endif /* SWR_USE_CUSTOM_STD_ALLOCATORS */

}    // namespace swr

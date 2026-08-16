/**
 * Software Rasterizer Playground.
 *
 * std unordered_map adapter.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <unordered_map>

#include "containers/allocator.h"
#include "containers/hash.h"

namespace swr
{

#if SWR_USE_CUSTOM_STD_ALLOCATORS

template<
  typename K,
  typename V,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
using unordered_map =
  std::unordered_map<
    K,
    V,
    swr::hash<K>,
    swr::equal_to<K>,
    swr::StdAllocator<
      std::pair<const K, V>,
      MemoryTag::UnorderedMap,
      Domain>>;

#else /* SWR_USE_CUSTOM_STD_ALLOCATORS */

template<
  typename K,
  typename V,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
using unordered_map =
  std::unordered_map<
    K,
    V,
    swr::hash<K>,
    swr::equal_to<K>,
    std::allocator<
      std::pair<const K, V>>>;

#endif /* SWR_USE_CUSTOM_STD_ALLOCATORS */

}    // namespace swr

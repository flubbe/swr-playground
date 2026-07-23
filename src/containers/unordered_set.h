/**
 * Software Rasterizer Playground.
 *
 * std unordered_set adapter.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <unordered_set>

#include "containers/allocator.h"

namespace swr
{

template<
  typename K,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
using unordered_set =
  std::unordered_set<
    K,
    std::hash<K>,
    std::equal_to<K>,
    swr::StdAllocator<
      K,
      MemoryTag::UnorderedSet,
      Domain>>;

}    // namespace swr

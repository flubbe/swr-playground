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

namespace swr
{

template<typename K, typename V>
using unordered_map =
  std::unordered_map<
    K,
    V,
    std::hash<K>,
    std::equal_to<K>,
    swr::StdAllocator<
      std::pair<const K, V>,
      MemoryTag::UnorderedMap>>;

}    // namespace swr

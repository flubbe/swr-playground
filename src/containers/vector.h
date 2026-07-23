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

template<
  typename T,
  memory::MemoryDomain Domain = memory::MemoryDomain::Heap>
using vector = std::vector<
  T,
  swr::StdAllocator<
    T,
    MemoryTag::Vector,
    Domain>>;

}    // namespace swr

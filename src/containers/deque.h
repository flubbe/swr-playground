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

template<typename T>
using deque = std::deque<
  T,
  swr::StdAllocator<
    T, MemoryTag::Deque>>;

}    // namespace swr

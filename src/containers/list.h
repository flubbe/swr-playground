/**
 * Software Rasterizer Playground.
 *
 * std list adapter.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <list>

#include "containers/allocator.h"

namespace swr
{

template<typename T>
using list = std::list<
  T,
  swr::StdAllocator<
    T,
    MemoryTag::List>>;

}    // namespace swr

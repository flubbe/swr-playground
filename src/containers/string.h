/**
 * Software Rasterizer Playground.
 *
 * std string adapter.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <string>

#include "containers/allocator.h"

namespace swr
{

using string = std::basic_string<
  char,
  std::char_traits<char>,
  swr::StdAllocator<
    char,
    MemoryTag::String>>;

}    // namespace swr
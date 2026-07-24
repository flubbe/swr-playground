/**
 * Software Rasterizer Playground.
 *
 * Utility functions.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <bit>
#include <cstddef>
#include <cassert>
#include <type_traits>

namespace memory
{

/**
 * Helper to round up to an alignment.
 *
 * @param x The value to round up.
 * @param align Alignment, has to be a power of two.
 */
constexpr std::size_t align_up(
  std::size_t x,
  std::size_t alignment)
{
    assert(std::has_single_bit(alignment));
    return (x + alignment - 1) & ~(alignment - 1);
}

/**
 * Align a pointer.
 *
 * @param x The pointer to align.
 * @param align Alignment, has to be a power of two.
 */
template<typename T>
    requires std::is_pointer_v<T>
constexpr T align(
  T x,
  std::size_t alignment)
{
    assert(std::has_single_bit(alignment));

    auto value = reinterpret_cast<std::uintptr_t>(x);
    value = (value + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<T>(value);
}

}    // namespace memory

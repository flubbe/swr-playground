/**
 * Software Rasterizer Playground.
 *
 * Common flag types.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace reflect
{

enum class PropertyFlags : std::uint32_t
{
    None = 0,
    ReadOnly = 1,
};

inline PropertyFlags operator|(
  PropertyFlags a,
  PropertyFlags b)
{
    using T = std::underlying_type_t<PropertyFlags>;
    return static_cast<PropertyFlags>(
      static_cast<T>(a) | static_cast<T>(b));
}

inline PropertyFlags operator&(
  PropertyFlags a,
  PropertyFlags b)
{
    using T = std::underlying_type_t<PropertyFlags>;
    return static_cast<PropertyFlags>(
      static_cast<T>(a) & static_cast<T>(b));
}

}    // namespace reflect

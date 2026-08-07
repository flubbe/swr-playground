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

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "containers/string.h"

/*
 * Strings.
 */

/**
 * Convert an ASCII string to lower-case ASCII
 * and return a copy.
 *
 * @param value The string.
 * @returns Returns the lower-case string copy.
 */
inline swr::string to_lower_copy(
  std::string_view value)
{
    swr::string copied_value{
      value.data(),
      value.size()};

    std::ranges::transform(
      copied_value,
      copied_value.begin(),
      [](unsigned char c) -> unsigned char
      {
          if(c >= 'A' && c <= 'Z')
          {
              return c - 'A' + 'a';
          }
          return c;
      });

    return copied_value;
}

/*
 * Safe casting.
 */

/**
 * Safely cast a value to another type.
 *
 * @param value The value to cast.
 * @returns Returns the input value for the new type.
 * @throws Throws a `std::out_of_range` exception if the value does not fit into the target type.
 */
template<typename T, typename S>
    requires(std::is_integral_v<T> && std::is_integral_v<S>)
constexpr T numeric_cast(S value)
{
    if constexpr(std::is_signed_v<S> == std::is_signed_v<T>)
    {
        using ComparisonType = std::common_type_t<S, T>;
        if(static_cast<ComparisonType>(value) < static_cast<ComparisonType>(std::numeric_limits<T>::min())
           || static_cast<ComparisonType>(value) > static_cast<ComparisonType>(std::numeric_limits<T>::max()))
        {
            throw std::out_of_range{"Value out of range of target type."};
        }
    }
    else if constexpr(std::is_signed_v<S> && !std::is_signed_v<T>)
    {
        if(value < 0
           || static_cast<std::make_unsigned_t<S>>(value) > std::numeric_limits<T>::max())
        {
            throw std::out_of_range{"Value out of range of target type."};
        }
    }
    else
    {
        if(value > static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::max()))
        {
            throw std::out_of_range{"Value out of range of target type."};
        }
    }

    return static_cast<T>(value);
}

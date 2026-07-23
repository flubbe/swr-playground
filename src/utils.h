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
#include <ranges>
#include <string>

#include "containers/string.h"

/**
 * Convert an ASCII string to lower-case ASCII
 * and return a copy.
 *
 * @param value The string.
 * @returns Returns the lower-case string copy.
 */
swr::string to_lower_copy(
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

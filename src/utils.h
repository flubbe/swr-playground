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

#include <ranges>
#include <string>

/**
 * Convert an ASCII string to lower-case ASCII
 * and return a copy.
 *
 * @param value The string.
 * @return Returns the lower-case string copy.
 */
inline std::string to_lower_copy(
  std::string value)
{
    std::ranges::transform(
      value,
      value.begin(),
      [](unsigned char c) -> unsigned char
      {
          if(c >= 'A' && c <= 'Z')
          {
              return c - 'A' + 'a';
          }
          return c;
      });

    return value;
}

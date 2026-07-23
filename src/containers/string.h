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
#include <type_traits>

#include "containers/allocator.h"

namespace swr
{

using string = std::basic_string<
  char,
  std::char_traits<char>,
  swr::StdAllocator<
    char,
    MemoryTag::String>>;

namespace detail
{

template<typename T>
    requires std::is_same_v<T, std::string>
             || std::is_same_v<T, string>
T string_from(
  std::string_view value)
{
    return {value.data(), value.size()};
}

}    // namespace detail

/**
 * Construct a string from `std::string_view`.
 *
 * @param s The input string.
 * @returns Returns an `swr::string`.
 */
inline string string_from(
  std::string_view s)
{
    return detail::string_from<string>(s);
}

/**
 * Construct a `std::string` from `std::string_view`.
 *
 * @param s The input string.
 * @returns Returns an `swr::string`.
 */
inline std::string std_string_from(
  std::string_view s)
{
    return detail::string_from<std::string>(s);
}

}    // namespace swr

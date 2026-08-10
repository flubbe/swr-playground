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
#include "containers/hash.h"

namespace swr
{

#if SWR_USE_CUSTOM_STD_ALLOCATORS

#    define SWR_CUSTOM_STRING_TYPE 1

using string = std::basic_string<
  char,
  std::char_traits<char>,
  swr::StdAllocator<
    char,
    MemoryTag::String>>;

#else /* SWR_USE_CUSTOM_STD_ALLOCATORS */

using string = std::string;

#endif /* SWR_USE_CUSTOM_STD_ALLOCATORS */

/*
 * Hashing and comparisons.
 */

template<>
struct hash<string>
{
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(
      const char* str) const noexcept
    {
        return hash_type{}(str);
    }

    std::size_t operator()(
      std::string_view str) const noexcept
    {
        return hash_type{}(str);
    }

    std::size_t operator()(
      const string& str) const noexcept
    {
        return hash_type{}(str);
    }
};

template<>
struct equal_to<string>
{
    using is_transparent = void;

    bool operator()(
      std::string_view lhs,
      std::string_view rhs) const noexcept
    {
        return lhs == rhs;
    }
};

namespace detail
{

template<typename T>
    requires std::is_same_v<T, std::string>
#if SWR_USE_CUSTOM_STD_ALLOCATORS
             || std::is_same_v<T, string>
#endif
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
/**
 * Software Rasterizer Playground.
 *
 * formatters into `swr::string`.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <format>
#include <version>

#include "containers/string.h"
#include "containers/vector.h"

namespace swr
{

#if SWR_USE_CUSTOM_STD_ALLOCATORS

template<typename... Args>
string format(
  std::format_string<Args...> fmt,
  Args&&... args)
{
    string result;

    std::format_to(
      std::back_inserter(result),
      fmt,
      std::forward<Args>(args)...);

    return result;
}

#else

template<typename... Args>
auto format(
  std::format_string<Args...> fmt,
  Args&&... args)
{
    return std::format(
      fmt,
      std::forward<Args>(args)...);
}

#endif

}    // namespace swr

/*
 * Enable if C++23 range formatting is missing.
 */

#if !defined(__cpp_lib_format_ranges)

namespace std
{
template<
  typename T,
  typename CharT>
struct formatter<swr::vector<T>, CharT>
: formatter<std::string_view, CharT>
{
    template<typename FormatContext>
    auto format(
      const vector<T>& vec,
      FormatContext& ctx) const
    {
        swr::string result = "[";
        for(size_t i = 0; i < vec.size(); ++i)
        {
            if(i > 0)
            {
                result += ", ";
            }
            result += std::format("{}", vec[i]);
        }
        result += "]";

        return formatter<string_view, CharT>::format(result, ctx);
    }
};

}    // namespace std

#endif /* !defined(__cpp_lib_format_ranges) */

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

#include "containers/string.h"

namespace swr
{

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

}    // namespace swr

/**
 * Software Rasterizer Playground.
 *
 * Exception definitions for object reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

namespace reflect
{

/** Thrown when an object does not satisfy an `is_a` type check. */
class instance_error : public std::runtime_error
{
public:
    explicit instance_error(
      std::string_view message)
    : std::runtime_error{
        std::format(
          "instance error: {}",
          message)}
    {
    }
};

}    // namespace reflect
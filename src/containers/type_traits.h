/**
 * Software Rasterizer Playground.
 *
 * type traits.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <type_traits>

namespace swr
{

/** Same as `std::false_type`, but taking a parameter argument. */
template<typename T>
struct false_type : public std::false_type
{
};

template<typename T>
inline constexpr bool false_type_v = false_type<T>::value;

}    // namespace swr

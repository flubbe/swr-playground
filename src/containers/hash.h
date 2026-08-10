/**
 * Software Rasterizer Playground.
 *
 * Hash.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <functional>

namespace swr
{

template<typename T>
struct hash : std::hash<T>
{
};

template<typename T>
struct equal_to : std::equal_to<T>
{
};

}    // namespace swr

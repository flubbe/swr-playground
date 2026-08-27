/**
 * Software Rasterizer Playground.
 *
 * Projection type.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstdint>

/** Projection type. */
enum class ProjectionType : std::uint8_t
{
    Perspective, /** Perspective projection. */
    Orthographic /** Orthographic projection. */
};

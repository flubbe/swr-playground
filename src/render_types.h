/**
 * Software Rasterizer Playground.
 *
 * Render-facing shared types.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstdint>

/** Part of a mesh using one material. */
struct MeshSection
{
    /** Mesh handle. */
    std::uint32_t mesh_handle{0};

    /** Material handle. */
    std::uint32_t material_handle{0};
};

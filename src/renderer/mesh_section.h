/**
 * Software Rasterizer Playground.
 *
 * A mesh section, that is, a mesh handle with materials and potentially other metadata.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <ml/all.h>

#include "resolvable_material.h"
#include "types.h"

/** Part of a mesh using one material. */
struct MeshSection
{
    /** Mesh handle. */
    MeshHandle mesh_handle{};

    /** Material path. */
    swr::string material_path;

    /** Material handle. */
    MaterialHandle material_handle{};

    /** Base color used by the lighting shader. */
    ml::vec4 color{1.f, 1.f, 1.f, 1.f};

    /*
     * Metadata.
     */

    /** Triangle count in this level of detail. */
    std::size_t triangle_count{0};
};

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

#include "assets/path.h"
#include "meshes/mesh.h"
#include "material.h"
#include "types.h"

/*
 * Forward declarations.
 */

struct AssetResolver;

/** Mesh section at a specific Level of Detail. */
struct SectionLOD
{
    /** GPU geometry handle for this LOD. */
    MeshHandle mesh_handle;

    /** Triangle count for this LOD. */
    std::size_t triangle_count{0};
};

/** Part of a mesh using one material. */
struct MeshSection
{
    /*
     * Serialized.
     */

    /** Base color used by the lighting shader. */
    ml::vec4 color{1.f, 1.f, 1.f, 1.f};

    /*
     * Runtime.
     */

    /** Mesh LODs. */
    swr::vector<SectionLOD> lods;

    /** Material reference. */
    MaterialRef material;

    /*
     * Generated metadata.
     */

    /** Mesh bounds. */
    MeshBounds bounds;

    /**
     * Selects the appropriate LOD index based on screen coverage and
     * target triangle density.
     *
     * @param projected_pixel_area Estimated projected pixel are of the section.
     * @param target_pixels_per_triangle Target pixels per triangle.
     * @returns Returns the selected LOD.
     */
    [[nodiscard]]
    std::size_t select_lod(
      float projected_pixel_area,
      float target_pixels_per_triangle) const noexcept;
};

/**
 * Software Rasterizer Playground.
 *
 * Level of detail support for meshes.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "containers/vector.h"
#include "meshes/simplifier.h"

/** LOD generation result. */
struct StaticMeshLodBuildResult
{
    /** Generated meshes. */
    swr::vector<MeshData> lod_meshes;

    /** Statistics. */
    swr::vector<MeshSimplifyStats> simplify_stats;
};

struct StaticMeshLodBuildSettings
{
    /** Triangle fractions to generate. */
    swr::vector<float> triangle_fractions{
      1.0f,      // LOD0: Original (Near camera)
      0.5f,      // LOD1: ~50% reduction
      0.25f,     // LOD2: ~75% reduction
      0.125f,    // LOD3: ~87.5% reduction
      0.03f,     // LOD4: ~97% reduction (Extreme distance silhouette)
    };

    /** Prevent collapsing boundary edges. */
    bool preserve_boundaries{false};

    /** Recompute normals after simplification. */
    bool recompute_normals{true};
};

/**
 * Build LODs for a mesh.
 *
 * @param source The source mesh.
 * @param settings LOD settings.
 * @returns The mesh LODs.
 */
[[nodiscard]]
StaticMeshLodBuildResult build_static_mesh_lods(
  const MeshData& source,
  const StaticMeshLodBuildSettings& settings);

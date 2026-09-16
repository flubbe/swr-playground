/**
 * Software Rasterizer Playground.
 *
 * Level of detail support for meshes.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "lod.h"

StaticMeshLodBuildResult build_static_mesh_lods(
  const MeshData& source,
  const StaticMeshLodBuildSettings& settings)
{
    StaticMeshLodBuildResult result;
    result.lod_meshes.reserve(settings.triangle_fractions.size());
    result.simplify_stats.reserve(settings.triangle_fractions.size());

    const std::size_t source_triangle_count = source.indices.size() / 3;

    for(float triangle_fraction: settings.triangle_fractions)
    {
        const float fraction =
          std::clamp(triangle_fraction, 0.f, 1.f);
        const std::size_t target_triangles =
          std::max<std::size_t>(
            1,
            static_cast<std::size_t>(
              static_cast<float>(source_triangle_count) * fraction));

        MeshData lod_mesh;
        MeshSimplifyStats stats{
          .input_triangles = source_triangle_count,
          .output_triangles = source_triangle_count,
          .target_triangles = source_triangle_count,
        };

        if(fraction >= 1.f)
        {
            lod_mesh = source;
        }
        else
        {
            const MeshData* base_mesh = &source;
            if(!result.lod_meshes.empty())
            {
                const MeshData& previous_mesh =
                  result.lod_meshes.back();
                const std::size_t previous_triangle_count =
                  previous_mesh.indices.size() / 3;

                if(previous_triangle_count > target_triangles
                   && previous_triangle_count < source_triangle_count)
                {
                    base_mesh = &previous_mesh;
                }
            }

            const std::size_t base_triangle_count =
              base_mesh->indices.size() / 3;
            const float base_fraction =
              base_triangle_count == 0
                ? 1.f
                : static_cast<float>(target_triangles)
                    / static_cast<float>(base_triangle_count);

            MeshSimplifier simplifier;

            lod_mesh = simplifier.simplify(
              *base_mesh,
              MeshSimplifySettings{
                .target_triangle_fraction = base_fraction,
                .preserve_boundaries = settings.preserve_boundaries,
                .recompute_normals = settings.recompute_normals,
              });

            stats = simplifier.stats();
        }

        result.lod_meshes.push_back(std::move(lod_mesh));
        result.simplify_stats.push_back(stats);
    }

    return result;
}

#pragma once

#include <algorithm>
#include <utility>

#include "containers/vector.h"
#include "meshes/simplifier.h"

struct StaticMeshLodMesh
{
    MeshData mesh;
    float min_screen_height = 0.f;
};

struct StaticMeshLodBuildResult
{
    swr::vector<StaticMeshLodMesh> lod_meshes;
    swr::vector<MeshSimplifyStats> simplify_stats;
};

struct StaticMeshLodBuildEntry
{
    /** Fraction of source triangles to keep. */
    float triangle_fraction{1.f};

    /**
     * Minimum projected screen-height fraction required
     * before this LOD becomes active.
     *
     * Larger values = higher detail.
     */
    float min_screen_height{0.f};
};

struct StaticMeshLodBuildSettings
{
    /**
     * LODs to generate.
     *
     * Usually ordered from highest detail to lowest detail.
     */
    swr::vector<StaticMeshLodBuildEntry> lods{
      {
        .triangle_fraction = 1.f,
        .min_screen_height = 0.50f,
      },
      {
        .triangle_fraction = 0.5f,
        .min_screen_height = 0.18f,
      },
      {
        .triangle_fraction = 0.25f,
        .min_screen_height = 0.0f,
      },
    };

    /** Prevent collapsing boundary edges. */
    bool preserve_boundaries{false};

    /** Recompute normals after simplification. */
    bool recompute_normals{true};
};

class StaticMeshLodBuilder
{
public:
    [[nodiscard]]
    StaticMeshLodBuildResult build(
      const MeshData& source,
      const StaticMeshLodBuildSettings& settings)
    {
        StaticMeshLodBuildResult result;
        result.lod_meshes.reserve(settings.lods.size());
        result.simplify_stats.reserve(settings.lods.size());

        const std::size_t source_triangle_count = source.indices.size() / 3;

        for(const StaticMeshLodBuildEntry& entry: settings.lods)
        {
            const float fraction =
              std::clamp(entry.triangle_fraction, 0.f, 1.f);
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
                      result.lod_meshes.back().mesh;
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

            result.lod_meshes.push_back({
              .mesh = std::move(lod_mesh),
              .min_screen_height = entry.min_screen_height,
            });

            result.simplify_stats.push_back(stats);
        }

        return result;
    }
};

/**
 * Software Rasterizer Playground.
 *
 * A mesh section, that is, a mesh handle with materials and potentially other metadata.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "mesh_section.h"

std::size_t MeshSection::select_lod(
  float projected_pixel_area,
  float target_pixels_per_triangle) const noexcept
{
    if(lods.empty())
    {
        return 0;
    }

    std::size_t fallback = 0;
    if(target_pixels_per_triangle <= 0)
    {
        return fallback;
    }

    projected_pixel_area = std::max(0.0f, projected_pixel_area);

    bool found_renderable = false;

    for(std::size_t lod_index = 0; lod_index < lods.size(); ++lod_index)
    {
        const SectionLOD& lod = lods[lod_index];
        if(lod.triangle_count == 0)
        {
            continue;
        }

        fallback = lod_index;
        found_renderable = true;

        const float pixels_per_triangle =
          projected_pixel_area / static_cast<float>(lod.triangle_count);

        if(pixels_per_triangle >= target_pixels_per_triangle)
        {
            return lod_index;
        }
    }

    return found_renderable ? fallback : 0;
}

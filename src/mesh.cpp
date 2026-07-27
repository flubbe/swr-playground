/**
 * Software Rasterizer Playground.
 *
 * Mesh data implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cmath>
#include <utility>

#include "mesh.h"

void expand_bounds(
  MeshBounds& bounds,
  const ml::vec3& p)
{
    if(!bounds.valid)
    {
        bounds.min = p;
        bounds.max = p;
        bounds.valid = true;
        return;
    }

    bounds.min.x = std::min(bounds.min.x, p.x);
    bounds.min.y = std::min(bounds.min.y, p.y);
    bounds.min.z = std::min(bounds.min.z, p.z);
    bounds.max.x = std::max(bounds.max.x, p.x);
    bounds.max.y = std::max(bounds.max.y, p.y);
    bounds.max.z = std::max(bounds.max.z, p.z);
}

void expand_bounds(
  MeshBounds& bounds,
  const ml::vec4& vertex)
{
    expand_bounds(bounds, vertex.xyz());
}

void expand_bounds(
  MeshBounds& bounds,
  const MeshBounds& other)
{
    if(!other.valid)
    {
        return;
    }

    expand_bounds(bounds, other.min);
    expand_bounds(bounds, other.max);
}

MeshBounds calculate_mesh_bounds(
  const MeshData& mesh)
{
    MeshBounds bounds;
    for(const auto& vertex: mesh.vertices)
    {
        expand_bounds(bounds, vertex);
    }

    return bounds;
}

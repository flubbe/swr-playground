/**
 * Software Rasterizer Playground.
 *
 * Mesh data.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "ml/all.h"

/** Primitive type for a mesh. */
enum class PrimitiveType
{
    Triangles, /** List of triangles. */
    Lines      /** List of lines. */
};

/** Raw (CPU-side) mesh data. */
struct MeshData
{
    /** Primitive type. */
    PrimitiveType primitive_type{PrimitiveType::Triangles};

    /** Indices into the geometry buffers (defining e.g. triangles or lines). */
    std::vector<std::uint32_t> indices;

    /** Vertices. */
    std::vector<ml::vec4> vertices;

    /** Normals. */
    std::vector<ml::vec4> normals;
};

/** Axis-aligned mesh bounds in local mesh space. */
struct MeshBounds
{
    /** Minimum corner. */
    ml::vec3 min{};

    /** Maximum corner. */
    ml::vec3 max{};

    /** Whether the bounds contain at least one point. */
    bool valid{false};
};

namespace detail
{

inline void expand_bounds(
  MeshBounds& bounds,
  const ml::vec4& vertex)
{
    const ml::vec3 position{vertex.xyz()};
    if(!bounds.valid)
    {
        bounds.min = position;
        bounds.max = position;
        bounds.valid = true;
        return;
    }

    bounds.min.x = std::min(bounds.min.x, position.x);
    bounds.min.y = std::min(bounds.min.y, position.y);
    bounds.min.z = std::min(bounds.min.z, position.z);
    bounds.max.x = std::max(bounds.max.x, position.x);
    bounds.max.y = std::max(bounds.max.y, position.y);
    bounds.max.z = std::max(bounds.max.z, position.z);
}

}    // namespace detail

[[nodiscard]]
inline MeshBounds calculate_mesh_bounds(
  const MeshData& mesh)
{
    MeshBounds bounds;
    for(const auto& vertex: mesh.vertices)
    {
        detail::expand_bounds(bounds, vertex);
    }

    return bounds;
}

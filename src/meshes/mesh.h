/**
 * Software Rasterizer Playground.
 *
 * Mesh data and helpers.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "ml/all.h"

#include "containers/vector.h"

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

    /** Texture coordinates packed into xy. */
    std::vector<ml::vec4> texcoords;
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

/**
 * Expand the mesh bounds to include a point.
 *
 * @param bounds The bounds to expand.
 * @param v The point.
 */
void expand_bounds(
  MeshBounds& bounds,
  const ml::vec3& p);

/**
 * Expand the mesh bounds to include a point.
 *
 * @param bounds The bounds to expand.
 * @param v The `x, y, z` coordinates of `v` define the point.
 */
void expand_bounds(
  MeshBounds& bounds,
  const ml::vec4& vertex);

/**
 * Expand the mesh bounds to include the `other` bounds.
 *
 * @param bounds The bounds to be expanded.
 * @param other The bounds to include.
 */
void expand_bounds(
  MeshBounds& bounds,
  const MeshBounds& other);

/**
 * Calculate the AABB bounds of a mesh.
 *
 * @param mesh The mesh.
 * @returns Returns the mesh bounds.
 */
[[nodiscard]]
MeshBounds calculate_mesh_bounds(
  const MeshData& mesh);

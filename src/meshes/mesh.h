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

#include <ml/all.h>

#include "containers/vector.h"
#include "serialization/containers.h"
#include "serialization/math.h"

/** Primitive type for a mesh. */
enum class PrimitiveType
{
    Triangles, /** List of triangles. */
    Lines      /** List of lines. */
};

/**
 * Serialize a primitive type.
 *
 * @param ar The archive to use.
 * @param primitive_type The primitive type.
 * @returns The input archive.
 */
inline serial::Archive& operator&(
  serial::Archive& ar,
  PrimitiveType& primitive_type)
{
    auto value = std::to_underlying(primitive_type);
    ar & value;

    if(ar.is_reading())
    {
        primitive_type = static_cast<PrimitiveType>(value);
    }

    return ar;
}

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

/**
 * Serialize a mesh.
 *
 * @param ar The archive to use.
 * @param mesh The mesh data.
 * @returns The input archive.
 */
inline serial::Archive& operator&(
  serial::Archive& ar,
  MeshData& mesh)
{
    ar & mesh.primitive_type;
    ar & mesh.indices;
    ar & mesh.vertices;
    ar & mesh.normals;
    ar & mesh.texcoords;

    return ar;
}

/** Axis-aligned mesh bounds in local mesh space. */
struct MeshBounds
{
    /** Minimum corner. */
    ml::vec3 min{};

    /** Maximum corner. */
    ml::vec3 max{};

    /** Bounding sphere center. */
    ml::vec3 center{};

    /** Bounding sphere radius. */
    float radius{0.f};

    /** Whether the bounds contain at least one point. */
    bool valid{false};
};

/**
 * Serialize mesh bounds.
 *
 * @param ar The archive to use.
 * @param bounds The mesh bounds.
 * @returns The input archive.
 */
inline serial::Archive& operator&(
  serial::Archive& ar,
  MeshBounds& bounds)
{
    ar & bounds.min
      & bounds.max
      & bounds.center
      & bounds.radius
      & bounds.valid;
    return ar;
}

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

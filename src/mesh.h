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

#include <array>
#include <cstddef>
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

void expand_bounds(
  MeshBounds& bounds,
  const ml::vec3& position);

void expand_bounds(
  MeshBounds& bounds,
  const ml::vec4& vertex);

void expand_bounds(
  MeshBounds& bounds,
  const MeshBounds& other);

namespace detail
{

struct MeshSimplifyTriangle
{
    std::array<std::uint32_t, 3> indices{};
};

struct MeshSimplifyEdgeCandidate
{
    float cost{0.f};
    std::uint32_t a{0};
    std::uint32_t b{0};
};

struct MeshSimplifyEdgeCandidateCompare
{
    bool operator()(
      const MeshSimplifyEdgeCandidate& lhs,
      const MeshSimplifyEdgeCandidate& rhs) const;
};

struct MeshSimplifyQuadric
{
    float m[4][4]{};
};

MeshSimplifyQuadric make_mesh_simplify_quadric(
  const ml::vec4& plane);

void add_mesh_simplify_quadric(
  MeshSimplifyQuadric& target,
  const MeshSimplifyQuadric& source);

float evaluate_mesh_simplify_quadric(
  const MeshSimplifyQuadric& quadric,
  const ml::vec4& position);

bool solve_mesh_simplify_optimal_position(
  const MeshSimplifyQuadric& quadric,
  ml::vec3& output);

std::uint64_t make_mesh_simplify_edge_key(
  std::uint32_t a,
  std::uint32_t b);

ml::vec3 mesh_simplify_triangle_normal(
  const std::array<std::uint32_t, 3>& roots,
  const std::vector<ml::vec3>& positions);

}    // namespace detail

[[nodiscard]]
MeshBounds calculate_mesh_bounds(
  const MeshData& mesh);

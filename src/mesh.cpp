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
  const ml::vec3& position)
{
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

namespace detail
{

bool MeshSimplifyEdgeCandidateCompare::operator()(
  const MeshSimplifyEdgeCandidate& lhs,
  const MeshSimplifyEdgeCandidate& rhs) const
{
    return lhs.cost > rhs.cost;
}

MeshSimplifyQuadric make_mesh_simplify_quadric(
  const ml::vec4& plane)
{
    const float a = plane.x;
    const float b = plane.y;
    const float c = plane.z;
    const float d = plane.w;

    MeshSimplifyQuadric quadric;
    quadric.m[0][0] = a * a;
    quadric.m[0][1] = a * b;
    quadric.m[0][2] = a * c;
    quadric.m[0][3] = a * d;

    quadric.m[1][0] = b * a;
    quadric.m[1][1] = b * b;
    quadric.m[1][2] = b * c;
    quadric.m[1][3] = b * d;

    quadric.m[2][0] = c * a;
    quadric.m[2][1] = c * b;
    quadric.m[2][2] = c * c;
    quadric.m[2][3] = c * d;

    quadric.m[3][0] = d * a;
    quadric.m[3][1] = d * b;
    quadric.m[3][2] = d * c;
    quadric.m[3][3] = d * d;

    return quadric;
}

void add_mesh_simplify_quadric(
  MeshSimplifyQuadric& target,
  const MeshSimplifyQuadric& source)
{
    for(int row = 0; row < 4; ++row)
    {
        for(int col = 0; col < 4; ++col)
        {
            target.m[row][col] += source.m[row][col];
        }
    }
}

float evaluate_mesh_simplify_quadric(
  const MeshSimplifyQuadric& quadric,
  const ml::vec4& position)
{
    const float x = position.x;
    const float y = position.y;
    const float z = position.z;
    const float w = position.w;

    const float x0 = quadric.m[0][0] * x + quadric.m[0][1] * y
                     + quadric.m[0][2] * z + quadric.m[0][3] * w;
    const float x1 = quadric.m[1][0] * x + quadric.m[1][1] * y
                     + quadric.m[1][2] * z + quadric.m[1][3] * w;
    const float x2 = quadric.m[2][0] * x + quadric.m[2][1] * y
                     + quadric.m[2][2] * z + quadric.m[2][3] * w;
    const float x3 = quadric.m[3][0] * x + quadric.m[3][1] * y
                     + quadric.m[3][2] * z + quadric.m[3][3] * w;

    return x * x0 + y * x1 + z * x2 + w * x3;
}

bool solve_mesh_simplify_optimal_position(
  const MeshSimplifyQuadric& quadric,
  ml::vec3& output)
{
    const float a00 = quadric.m[0][0];
    const float a01 = quadric.m[0][1];
    const float a02 = quadric.m[0][2];
    const float a10 = quadric.m[1][0];
    const float a11 = quadric.m[1][1];
    const float a12 = quadric.m[1][2];
    const float a20 = quadric.m[2][0];
    const float a21 = quadric.m[2][1];
    const float a22 = quadric.m[2][2];

    const float b0 = quadric.m[0][3];
    const float b1 = quadric.m[1][3];
    const float b2 = quadric.m[2][3];

    const float det = a00 * (a11 * a22 - a12 * a21)
                      - a01 * (a10 * a22 - a12 * a20)
                      + a02 * (a10 * a21 - a11 * a20);

    if(std::abs(det) < 1e-12f)
    {
        return false;
    }

    const float inv_det = 1.0f / det;
    output.x = inv_det
               * ((a11 * a22 - a12 * a21) * -b0
                  - (a01 * a22 - a02 * a21) * -b1
                  + (a01 * a12 - a02 * a11) * -b2);
    output.y = inv_det
               * (-(a10 * a22 - a12 * a20) * -b0
                  + (a00 * a22 - a02 * a20) * -b1
                  - (a00 * a12 - a02 * a10) * -b2);
    output.z = inv_det
               * ((a10 * a21 - a11 * a20) * -b0
                  - (a00 * a21 - a01 * a20) * -b1
                  + (a00 * a11 - a01 * a10) * -b2);
    return true;
}

std::uint64_t make_mesh_simplify_edge_key(
  std::uint32_t a,
  std::uint32_t b)
{
    if(a > b)
    {
        std::swap(a, b);
    }

    return (static_cast<std::uint64_t>(a) << 32u)
           | static_cast<std::uint64_t>(b);
}

ml::vec3 mesh_simplify_triangle_normal(
  const std::array<std::uint32_t, 3>& roots,
  const swr::vector<ml::vec3>& positions)
{
    const ml::vec3 e0 = positions[roots[1]] - positions[roots[0]];
    const ml::vec3 e1 = positions[roots[2]] - positions[roots[0]];
    return e0.cross_product(e1);
}

}    // namespace detail

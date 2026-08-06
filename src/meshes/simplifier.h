/**
 * Software Rasterizer Playground.
 *
 * Mesh simplifier.
 *
 * Based on: M. Garland, P. S. Heckbert, "Surface Simplification Using Quadric Error Metrics" (1997),
 *           https://www.cs.cmu.edu/~garland/Papers/quadrics.pdf
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>

#include "containers/unordered_map.h"
#include "containers/unordered_set.h"
#include "containers/vector.h"
#include "meshes/mesh.h"

struct MeshSimplifySettings
{
    float target_triangle_fraction = 0.5f;
    bool preserve_boundaries = false;
    bool recompute_normals = true;
};

struct MeshSimplifyStats
{
    std::size_t input_triangles = 0;
    std::size_t output_triangles = 0;
    std::size_t target_triangles = 0;
    std::size_t boundary_vertices = 0;
    std::size_t queued_edges = 0;
    std::size_t accepted_collapses = 0;
    std::size_t rejected_collapses = 0;
};

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
      const MeshSimplifyEdgeCandidate& rhs) const
    {
        return lhs.cost > rhs.cost;
    }
};

struct MeshSimplifyQuadric
{
    ml::mat4x4 m{};

    float evaluate(
      const ml::vec4& position) const
    {
        return ml::dot(
          position, m * position);
    }
};

}    // namespace detail

class MeshSimplifier
{
    static constexpr float mesh_simplify_area_epsilon = 1e-12f;    // FIXME maybe adjust?

public:
    [[nodiscard]]
    MeshData simplify(
      const MeshData& mesh,
      const MeshSimplifySettings& settings);

    [[nodiscard]]
    const MeshSimplifyStats& stats() const;

private:
    static constexpr std::uint32_t unmapped =
      std::numeric_limits<std::uint32_t>::max();

    using EdgeMap =
      swr::unordered_map<
        std::uint64_t,
        swr::vector<std::size_t>>;

    using EdgeQueue =
      std::priority_queue<
        detail::MeshSimplifyEdgeCandidate,
        swr::vector<detail::MeshSimplifyEdgeCandidate>,
        detail::MeshSimplifyEdgeCandidateCompare>;

    const MeshData* source_mesh = nullptr;
    MeshSimplifySettings simplify_settings;
    MeshSimplifyStats simplify_stats;

    swr::vector<detail::MeshSimplifyTriangle> triangles;
    swr::vector<std::uint32_t> vertex_parents;
    swr::vector<ml::vec3> vertex_positions;
    swr::vector<bool> active_triangles;
    swr::vector<bool> boundary_vertices;
    swr::vector<swr::vector<std::size_t>> vertex_triangles;
    swr::vector<detail::MeshSimplifyQuadric> vertex_quadrics;
    swr::unordered_set<std::uint64_t> active_triangle_keys;
    float area_epsilon = mesh_simplify_area_epsilon;

private:
    void reset(
      const MeshData& mesh,
      const MeshSimplifySettings& settings);

    void build_triangle_list();

    [[nodiscard]]
    static float calculate_area_epsilon(
      const MeshData& mesh);

    [[nodiscard]]
    bool can_simplify() const;

    [[nodiscard]]
    std::uint32_t root_of(std::uint32_t vertex);

    [[nodiscard]]
    std::array<std::uint32_t, 3> triangle_roots(
      const detail::MeshSimplifyTriangle& triangle);

    [[nodiscard]]
    EdgeMap build_edges();

    [[nodiscard]]
    std::size_t rebuild_adjacency();

    std::size_t collapse_edge(
      std::uint32_t a,
      std::uint32_t b,
      const ml::vec3& collapse_position);

    void rebuild_quadrics();

    [[nodiscard]]
    detail::MeshSimplifyEdgeCandidate make_edge_candidate(
      std::uint32_t a,
      std::uint32_t b);

    [[nodiscard]]
    EdgeQueue build_edge_queue();

    [[nodiscard]]
    swr::vector<std::size_t> collect_affected_triangles(
      std::uint32_t a,
      std::uint32_t b);

    [[nodiscard]]
    bool collapse_preserves_triangle(
      const std::array<std::uint32_t, 3>& roots,
      std::uint32_t removed,
      std::uint32_t kept,
      const ml::vec3& collapse_position,
      const swr::unordered_set<std::uint64_t>& existing,
      const swr::unordered_set<std::uint64_t>& affected_keys,
      swr::unordered_set<std::uint64_t>& proposed);

    [[nodiscard]]
    bool can_collapse(
      std::uint32_t kept,
      std::uint32_t removed,
      const ml::vec3& collapse_position,
      const swr::vector<std::size_t>& affected);

    void simplify_until_target();

    [[nodiscard]]
    MeshData build_output_mesh();
};

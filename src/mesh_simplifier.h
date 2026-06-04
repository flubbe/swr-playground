#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mesh.h"

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
      std::unordered_map<
        std::uint64_t,
        std::vector<std::size_t>>;

    using EdgeQueue =
      std::priority_queue<
        detail::MeshSimplifyEdgeCandidate,
        std::vector<detail::MeshSimplifyEdgeCandidate>,
        detail::MeshSimplifyEdgeCandidateCompare>;

    const MeshData* source_mesh = nullptr;
    MeshSimplifySettings simplify_settings;
    MeshSimplifyStats simplify_stats;

    std::vector<detail::MeshSimplifyTriangle> triangles;
    std::vector<std::uint32_t> vertex_parents;
    std::vector<ml::vec3> vertex_positions;
    std::vector<bool> active_triangles;
    std::vector<bool> boundary_vertices;
    std::vector<std::vector<std::size_t>> vertex_triangles;
    std::vector<detail::MeshSimplifyQuadric> vertex_quadrics;
    std::unordered_set<std::uint64_t> active_triangle_keys;
    float area_epsilon = mesh_simplify_area_epsilon;

private:
    void reset(
      const MeshData& mesh,
      const MeshSimplifySettings& settings);

    void build_triangle_list();

    [[nodiscard]]
    static float calculate_area_epsilon(const MeshData& mesh);

    [[nodiscard]]
    bool can_simplify() const;

    [[nodiscard]]
    std::uint32_t root_of(std::uint32_t vertex);

    [[nodiscard]]
    std::array<std::uint32_t, 3> triangle_roots(
      const detail::MeshSimplifyTriangle& triangle);

    [[nodiscard]]
    static bool is_valid_triangle(
      const std::array<std::uint32_t, 3>& roots);

    [[nodiscard]]
    static std::uint64_t triangle_key(
      std::uint32_t a,
      std::uint32_t b,
      std::uint32_t c);

    [[nodiscard]]
    EdgeMap build_edges();

    [[nodiscard]]
    std::size_t rebuild_adjacency();

    void rebuild_quadrics();

    [[nodiscard]]
    detail::MeshSimplifyEdgeCandidate make_edge_candidate(
      std::uint32_t a,
      std::uint32_t b);

    [[nodiscard]]
    EdgeQueue build_edge_queue();

    [[nodiscard]]
    std::vector<std::size_t> collect_affected_triangles(
      std::uint32_t a,
      std::uint32_t b);

    [[nodiscard]]
    static std::array<std::uint32_t, 3> collapsed_roots(
      std::array<std::uint32_t, 3> roots,
      std::uint32_t removed,
      std::uint32_t kept);

    [[nodiscard]]
    bool collapse_preserves_triangle(
      const std::array<std::uint32_t, 3>& roots,
      std::uint32_t removed,
      std::uint32_t kept,
      const ml::vec3& collapse_position,
      const std::unordered_set<std::uint64_t>& existing,
      const std::unordered_set<std::uint64_t>& affected_keys,
      std::unordered_set<std::uint64_t>& proposed);

    [[nodiscard]]
    bool can_collapse(
      std::uint32_t kept,
      std::uint32_t removed,
      const ml::vec3& collapse_position,
      const std::vector<std::size_t>& affected);

    void simplify_until_target();

    [[nodiscard]]
    MeshData build_output_mesh();
};

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

    /** Minimal triangle count. */
    std::size_t min_triangle_count = 64uz;

    bool preserve_boundaries = true;
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
    /**
     * Simplifies a mesh down to a target triangle fraction using Quadric Error Metrics.
     *
     * @param mesh The source mesh data.
     * @param settings Configuration for target LOD size and boundary constraints.
     * @returns The simplified mesh data, or a copy of the source if simplification is skipped.
     */
    [[nodiscard]]
    MeshData simplify(
      const MeshData& mesh,
      const MeshSimplifySettings& settings);

    /**
     * Retrieves metrics and counts from the most recent simplification pass.
     *
     * @returns A struct containing triangle counts and collapse statistics.
     */
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

    /** Pointer to the original, unmodified mesh being simplified. */
    const MeshData* source_mesh = nullptr;

    /** Configuration parameters controlling target triangle fraction and constraints. */
    MeshSimplifySettings simplify_settings;

    /** Diagnostics and operational metrics for the last simplification pass. */
    MeshSimplifyStats simplify_stats;

    /** Working list of mesh triangles mapped to initial vertex indices. */
    swr::vector<detail::MeshSimplifyTriangle> triangles;

    /** Disjoint-set parent pointers for vertex merging and path compression. */
    swr::vector<std::uint32_t> vertex_parents;

    /** Current spatial positions of active and merged vertices. */
    swr::vector<ml::vec3> vertex_positions;

    /** Bitset flags indicating whether a triangle is currently active in the topology. */
    swr::vector<bool> active_triangles;

    /** Bitset flags marking vertices situated on mesh open boundaries. */
    swr::vector<bool> boundary_vertices;

    /** Adjacency lookup mapping each vertex to its connected triangle indices. */
    swr::vector<swr::vector<std::size_t>> vertex_triangles;

    /** Quadric error matrices accumulated per vertex for geometric error tracking. */
    swr::vector<detail::MeshSimplifyQuadric> vertex_quadrics;

    /** Set of canonical edge keys tracking unique active face connections. */
    swr::unordered_set<std::uint64_t> active_triangle_keys;

    /** Dynamic epsilon threshold used for face normal and area validation. */
    float area_epsilon = mesh_simplify_area_epsilon;

private:
    /*
     * Initialization and state management.
     */

    /**
     * Initializes internal structures, buffers, and statistics for a new simplification pass.
     *
     * @param mesh The source mesh.
     * @param settings Configuration settings for the run.
     */
    void reset(
      const MeshData& mesh,
      const MeshSimplifySettings& settings);

    /**
     * Populates the internal triangle list by filtering out invalid or out-of-bounds indices
     * from the source mesh.
     */
    void build_triangle_list();

    /**
     * Calculates a dynamic epsilon value for valid normal lengths based on the mesh bounding box
     * extent.
     *
     * @param mesh The mesh used to compute the geometric bounds.
     * @returns The scaled area epsilon threshold.
     */
    [[nodiscard]]
    static float calculate_area_epsilon(
      const MeshData& mesh);

    /**
     * Checks if the mesh topology and current targets allow for geometric simplification.
     *
     * @returns `true` if the mesh can be simplified further, `false` otherwise.
     */
    [[nodiscard]]
    bool can_simplify() const;

    /*
     * Topology and adjacency.
     */

    /**
     * Resolves the canonical vertex index for a given vertex, performing path compression on
     * parent pointers.
     *
     * @param vertex The queried vertex index.
     * @returns The root index of the merged vertex cluster.
     */
    [[nodiscard]]
    std::uint32_t root_of(std::uint32_t vertex);

    /**
     * Retrieves the resolved root indices for all three vertices of a given triangle.
     *
     * @param triangle The triangle to inspect.
     * @returns An array containing the three root vertex indices.
     */
    [[nodiscard]]
    std::array<std::uint32_t, 3> triangle_roots(
      const detail::MeshSimplifyTriangle& triangle);

    /**
     * Extracts and maps all directed edges from the currently active triangles.
     *
     * @returns A map associating canonical edge keys with lists of active triangle indices sharing them.
     */
    [[nodiscard]]
    EdgeMap build_edges();

    /**
     * Builds vertex-to-triangle adjacency lists and detects boundary vertices based on edge sharing.
     *
     * @returns The total number of currently active triangles.
     */
    [[nodiscard]]
    std::size_t build_adjacency();

    /**
     * Gathers a deduplicated list of active triangles connected to either of two vertices.
     *
     * @param a The first vertex index.
     * @param b The second vertex index.
     * @returns A vector of affected triangle indices.
     */
    [[nodiscard]]
    swr::vector<std::size_t> collect_affected_triangles(
      std::uint32_t a,
      std::uint32_t b);

    /*
     * Core simplification algorithm.
     */

    /**
     * Computes error quadric matrices for all active vertices and applies heavy penalty weights to boundary edges.
     *
     * @param edges The map of current active edges used to identify boundaries.
     */
    void rebuild_quadrics(
      const EdgeMap& edges);

    /**
     * Evaluates the quadric error cost of collapsing an edge and computes its optimal unified position.
     *
     * @param a The kept vertex index.
     * @param b The removed vertex index.
     * @returns An edge candidate structure containing the merged cost and vertex pair.
     */
    [[nodiscard]]
    detail::MeshSimplifyEdgeCandidate make_edge_candidate(
      std::uint32_t a,
      std::uint32_t b);

    /**
     * Computes base quadrics and populates a priority queue with all valid initial edge collapse candidates.
     *
     * @returns A priority queue ordering edge candidates by lowest error cost.
     */
    [[nodiscard]]
    EdgeQueue build_edge_queue();

    /**
     * Validates whether a specific proposed triangle collapse avoids flipping the face normal or
     * creating degenerate overlaps.
     *
     * @param roots The current triangle vertex roots.
     * @param removed The vertex being merged.
     * @param kept The vertex being kept.
     * @param collapse_position The proposed optimal position of the unified vertex.
     * @param existing Set of active canonical triangle keys in the mesh.
     * @param affected_keys Set of keys for triangles directly affected by this local collapse.
     * @param proposed Set of newly formed triangle keys post-collapse.
     * @returns `true` if the collapse is topologically and geometrically safe.
     */
    [[nodiscard]]
    bool collapse_preserves_triangle(
      const std::array<std::uint32_t, 3>& roots,
      std::uint32_t removed,
      std::uint32_t kept,
      const ml::vec3& collapse_position,
      const swr::unordered_set<std::uint64_t>& existing,
      const swr::unordered_set<std::uint64_t>& affected_keys,
      swr::unordered_set<std::uint64_t>& proposed);

    /**
     * Determines if a full edge collapse is globally safe by ensuring boundaries are not pinched
     * and affected geometry remains valid.
     *
     * @param kept The vertex to keep.
     * @param removed The vertex to collapse into the kept vertex.
     * @param collapse_position The unified target position.
     * @param affected The list of all triangle indices sharing either vertex.
     * @returns `true` if the collapse can safely proceed.
     */
    [[nodiscard]]
    bool can_collapse(
      std::uint32_t kept,
      std::uint32_t removed,
      const ml::vec3& collapse_position,
      const swr::vector<std::size_t>& affected);

    /**
     * Merges two vertices, sums their quadric matrices, and patches triangle adjacency lists.
     *
     * @param a The vertex index to keep.
     * @param b The vertex index to merge into the kept vertex.
     * @param collapse_position The new physical position of the merged vertex.
     * @returns The number of triangles degenerated (removed) during this collapse.
     */
    std::size_t collapse_edge(
      std::uint32_t a,
      std::uint32_t b,
      const ml::vec3& collapse_position);

    /**
     * Iteratively processes the cheapest valid edge collapses from the queue until the
     * target triangle fraction is reached.
     */
    void simplify_until_target();

    /**
     * Constructs the final mesh output by collapsing index buffers and conditionally recomputing normals.
     *
     * @returns The newly simplified mesh representation.
     */
    [[nodiscard]]
    MeshData build_output_mesh();

    /**
     * Evaluates valid edge collapse candidates surrounding a unified vertex and pushes them into
     * the priority queue.
     *
     * @param v The canonical root vertex index whose neighbor edges are being re-evaluated.
     * @param queue The active edge priority queue receiving new candidates.
     */
    void push_vertex_edges_to_queue(
      std::uint32_t v,
      EdgeQueue& queue);
};

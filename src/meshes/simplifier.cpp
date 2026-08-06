/**
 * Software Rasterizer Playground.
 *
 * Mesh simplifier implementation.
 *
 * Based on: M. Garland, P. S. Heckbert, "Surface Simplification Using Quadric Error Metrics" (1997),
 *           https://www.cs.cmu.edu/~garland/Papers/quadrics.pdf
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <utility>

#include "meshes/simplifier.h"

namespace
{

detail::MeshSimplifyQuadric make_mesh_simplify_quadric(
  const ml::plane& plane)
{
    return {
      .m = ml::outer_product(plane, plane)};
}

void add_mesh_simplify_quadric(
  detail::MeshSimplifyQuadric& target,
  const detail::MeshSimplifyQuadric& source)
{
    target.m += source.m;
}

/**
 * Solves for the optimal vertex position that minimizes quadric error:
 * ```
 *     A x = -b
 * ```
 * where `A` is the upper-left 3×3 block of `q.m` and `b` is formed from the
 * first three elements of the fourth row of `q`.
 *
 * @param q Quadric.
 * @param result The computed optimal position vector.
 * @param epsilon Singularity threshold passed to matrix inversion.
 * @returns `true` if solvable, `false` if the system is near-singular/degenerate.
 */
[[nodiscard]]
bool solve_quadric_optimal_vertex(
  const detail::MeshSimplifyQuadric& q,
  ml::vec3& result,
  float epsilon = ml::epsilon)
{
    ml::mat3x3 A{
      q.m.rows[0].xyz(),
      q.m.rows[1].xyz(),
      q.m.rows[2].xyz()};

    if(!A.invert(epsilon))
    {
        return false;
    }

    result = -A * q.m.rows[3].xyz();
    return true;
}

/**
 * Create a canonical edge key by bit-packing the indices.
 *
 * @param a An edge vertex index.
 * @param b An edge vertex index.
 * @returns Returns a canonical triangle key.
 */
[[nodiscard]]
std::uint64_t pack_edge_key(
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

/**
 * Unpack an edge key and return the edge indices.
 *
 * @param edge_key The edge key.
 * @returns Returns a pair `(a, b)` of indices.
 */
[[nodiscard]]
std::pair<std::uint32_t, std::uint32_t> unpack_edge_key(
  std::uint64_t edge_key)
{
    return {
      static_cast<std::uint32_t>(edge_key >> 32u),
      static_cast<std::uint32_t>(edge_key & 0xffffffffu)};
}

/**
 * Calculate the normal of a triangle.
 *
 * @param indices The triange's vertex indices as entries into `positions`.
 * @param positions Buffer holding all mesh vertex positions.
 * @returns Returns the triangle normal.
 */
[[nodiscard]]
ml::vec3 calculate_triangle_normal(
  const std::array<std::uint32_t, 3>& indices,
  const swr::vector<ml::vec3>& positions)
{
    const ml::vec3 e0 = positions[indices[1]] - positions[indices[0]];
    const ml::vec3 e1 = positions[indices[2]] - positions[indices[0]];
    return e0.cross_product(e1);
}

/** Check if all triangle indices are different. */
[[nodiscard]]
bool is_valid_triangle(
  const std::array<std::uint32_t, 3>& indices)
{
    return indices[0] != indices[1]
           && indices[0] != indices[2]
           && indices[1] != indices[2];
}

/**
 * Calculate a canonical triangle key by bit-packing the indices.
 *
 * @param a A triangle vertex index.
 * @param b A triangle vertex index.
 * @param c A triangle vertex index.
 * @returns Returns a canonical triangle key.
 */
[[nodiscard]]
std::uint64_t triangle_key(
  std::uint32_t a,
  std::uint32_t b,
  std::uint32_t c)
{
    if(a > b)
    {
        std::swap(a, b);
    }

    if(b > c)
    {
        std::swap(b, c);
    }

    if(a > b)
    {
        std::swap(a, b);
    }

    return (static_cast<std::uint64_t>(a) << 42)
           | (static_cast<std::uint64_t>(b) << 21)
           | static_cast<std::uint64_t>(c);
}

[[nodiscard]]
std::array<std::uint32_t, 3> collapsed_roots(
  std::array<std::uint32_t, 3> roots,
  std::uint32_t removed,
  std::uint32_t kept)
{
    for(auto& root: roots)
    {
        if(root == removed)
        {
            root = kept;
        }
    }

    return roots;
}

}    // namespace

MeshData MeshSimplifier::simplify(
  const MeshData& mesh,
  const MeshSimplifySettings& settings)
{
    reset(mesh, settings);

    if(!can_simplify())
    {
        return mesh;
    }

    simplify_until_target();

    if(simplify_stats.accepted_collapses == 0)
    {
        return mesh;
    }

    return build_output_mesh();
}

const MeshSimplifyStats& MeshSimplifier::stats() const
{
    return simplify_stats;
}

void MeshSimplifier::reset(
  const MeshData& mesh,
  const MeshSimplifySettings& settings)
{
    source_mesh = &mesh;
    simplify_settings = settings;
    simplify_settings.target_triangle_fraction =
      std::clamp(simplify_settings.target_triangle_fraction, 0.f, 1.f);

    simplify_stats = {};

    triangles.clear();
    vertex_parents.clear();
    vertex_positions.clear();
    active_triangles.clear();
    boundary_vertices.clear();
    vertex_triangles.clear();
    vertex_quadrics.clear();
    active_triangle_keys.clear();

    const std::size_t vertex_count = mesh.vertices.size();
    area_epsilon = calculate_area_epsilon(mesh);

    vertex_parents.resize(vertex_count);
    vertex_positions.resize(vertex_count);
    boundary_vertices.resize(vertex_count, false);
    vertex_triangles.resize(vertex_count);
    vertex_quadrics.resize(vertex_count);

    for(std::uint32_t i = 0; i < vertex_count; ++i)
    {
        vertex_parents[i] = i;
        vertex_positions[i] = mesh.vertices[i].xyz();
    }

    build_triangle_list();

    active_triangles.assign(triangles.size(), true);

    simplify_stats.input_triangles = triangles.size();
    simplify_stats.target_triangles =
      std::max(
        settings.min_triangle_count,
        static_cast<std::size_t>(
          static_cast<float>(triangles.size())
          * simplify_settings.target_triangle_fraction));
}

void MeshSimplifier::build_triangle_list()
{
    const auto& mesh = *source_mesh;
    const std::size_t vertex_count = mesh.vertices.size();

    triangles.reserve(mesh.indices.size() / 3);

    for(std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
    {
        const std::uint32_t i0 = mesh.indices[i + 0];
        const std::uint32_t i1 = mesh.indices[i + 1];
        const std::uint32_t i2 = mesh.indices[i + 2];

        if(i0 >= vertex_count
           || i1 >= vertex_count
           || i2 >= vertex_count
           || !is_valid_triangle({i0, i1, i2}))
        {
            continue;
        }

        triangles.push_back({.indices = {i0, i1, i2}});
    }
}

float MeshSimplifier::calculate_area_epsilon(
  const MeshData& mesh)
{
    const MeshBounds bounds = calculate_mesh_bounds(mesh);
    if(!bounds.valid)
    {
        return mesh_simplify_area_epsilon;
    }

    const ml::vec3 extent = bounds.max - bounds.min;
    const float diagonal_length_squared = extent.length_squared();
    const float scaled_epsilon =
      diagonal_length_squared * diagonal_length_squared * 1e-12f;    // FIXME magic number

    return std::max(scaled_epsilon, mesh_simplify_area_epsilon);
}

bool MeshSimplifier::can_simplify() const
{
    return source_mesh->primitive_type == PrimitiveType::Triangles
           && !source_mesh->vertices.empty()
           && triangles.size() >= 2
           && simplify_settings.target_triangle_fraction < 1.f
           && simplify_stats.target_triangles < triangles.size();
}

std::uint32_t MeshSimplifier::root_of(
  std::uint32_t vertex)
{
    while(vertex_parents[vertex] != vertex)
    {
        vertex_parents[vertex] = vertex_parents[vertex_parents[vertex]];
        vertex = vertex_parents[vertex];
    }

    return vertex;
}

std::array<std::uint32_t, 3> MeshSimplifier::triangle_roots(
  const detail::MeshSimplifyTriangle& triangle)
{
    return {
      root_of(triangle.indices[0]),
      root_of(triangle.indices[1]),
      root_of(triangle.indices[2]),
    };
}

MeshSimplifier::EdgeMap MeshSimplifier::build_edges()
{
    EdgeMap edges;

    for(std::size_t triangle_index = 0;
        triangle_index < triangles.size();
        ++triangle_index)
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        if(!is_valid_triangle(roots))
        {
            continue;
        }

        edges[pack_edge_key(roots[0], roots[1])]
          .emplace_back(triangle_index);
        edges[pack_edge_key(roots[1], roots[2])]
          .emplace_back(triangle_index);
        edges[pack_edge_key(roots[2], roots[0])]
          .emplace_back(triangle_index);
    }

    return edges;
}

std::size_t MeshSimplifier::rebuild_adjacency()
{
    for(auto& list: vertex_triangles)
    {
        list.clear();
    }

    std::fill(
      boundary_vertices.begin(),
      boundary_vertices.end(),
      false);

    std::size_t active_count = 0;
    active_triangle_keys.clear();
    active_triangle_keys.reserve(triangles.size());

    for(std::size_t triangle_index = 0;
        triangle_index < triangles.size();
        ++triangle_index)
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        if(!is_valid_triangle(roots))
        {
            active_triangles[triangle_index] = false;
            continue;
        }

        vertex_triangles[roots[0]].push_back(triangle_index);
        vertex_triangles[roots[1]].push_back(triangle_index);
        vertex_triangles[roots[2]].push_back(triangle_index);
        active_triangle_keys.insert(
          triangle_key(roots[0], roots[1], roots[2]));

        ++active_count;
    }

    simplify_stats.boundary_vertices = 0;

    for(const auto& [edge_key, faces]: build_edges())
    {
        if(faces.size() != 2)
        {
            const auto [a, b] = unpack_edge_key(edge_key);
            boundary_vertices[a] = true;
            boundary_vertices[b] = true;
        }
    }

    for(const bool is_boundary: boundary_vertices)
    {
        simplify_stats.boundary_vertices += is_boundary ? 1 : 0;
    }

    return active_count;
}

std::size_t MeshSimplifier::collapse_edge(
  std::uint32_t a,
  std::uint32_t b,
  const ml::vec3& collapse_position)
{
    vertex_parents[b] = a;
    vertex_positions[a] = collapse_position;
    vertex_quadrics[a].m += vertex_quadrics[b].m;
    boundary_vertices[a] =
      boundary_vertices[a] || boundary_vertices[b];

    std::size_t removed_triangles = 0;

    auto affected = collect_affected_triangles(a, b);

    // Remove old adjacency.
    for(auto triangle_index: affected)
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        for(auto v: roots)
        {
            auto& list = vertex_triangles[v];
            std::erase(list, triangle_index);
        }
    }

    // Reinsert updated triangles.
    for(auto triangle_index: affected)
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        if(!is_valid_triangle(roots))
        {
            active_triangles[triangle_index] = false;
            ++removed_triangles;
            continue;
        }

        for(auto v: roots)
        {
            vertex_triangles[v].push_back(triangle_index);
        }
    }

    return removed_triangles;
}

void MeshSimplifier::rebuild_quadrics(
  const EdgeMap& edges)
{
    for(auto& quadric: vertex_quadrics)
    {
        quadric = {};
    }

    for(std::size_t triangle_index = 0;
        triangle_index < triangles.size();
        ++triangle_index)
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        if(!is_valid_triangle(roots))
        {
            continue;
        }

        const ml::vec3 normal = calculate_triangle_normal(roots, vertex_positions);

        // if(normal.length_squared() <= area_epsilon)
        // {
        //     continue;
        // }

        const ml::vec3 n = normal.normalized();
        const float d = -n.dot_product(vertex_positions[roots[0]]);

        const auto q = make_mesh_simplify_quadric({n.x, n.y, n.z, d});

        add_mesh_simplify_quadric(vertex_quadrics[roots[0]], q);
        add_mesh_simplify_quadric(vertex_quadrics[roots[1]], q);
        add_mesh_simplify_quadric(vertex_quadrics[roots[2]], q);
    }

    // Boundary Constraint Quadrics
    for(const auto& [edge_key, faces]: edges)
    {
        if(faces.size() == 1)    // Boundary edge!
        {
            const auto [a, b] = unpack_edge_key(edge_key);

            const ml::vec3 pa = vertex_positions[a];
            const ml::vec3 pb = vertex_positions[b];

            ml::vec3 edge_dir = pb - pa;
            const float len_sq = edge_dir.length_squared();
            if(len_sq <= 1e-12f)
            {
                continue;
            }
            edge_dir = edge_dir * (1.0f / std::sqrt(len_sq));

            // Face normal of the single triangle on this boundary
            const auto roots = triangle_roots(triangles[faces[0]]);
            const ml::vec3 face_normal = calculate_triangle_normal(roots, vertex_positions).normalized();

            // Constraint plane perpendicular to the face AND aligned along the boundary edge
            const ml::vec3 constraint_normal = edge_dir.cross_product(face_normal).normalized();
            const float d = -constraint_normal.dot_product(pa);

            // Construct constraint plane quadric weighted heavily (Garland & Heckbert suggest ~240x weight)
            auto boundary_q = make_mesh_simplify_quadric({constraint_normal.x, constraint_normal.y, constraint_normal.z, d});
            boundary_q.m *= 200.0f;

            add_mesh_simplify_quadric(vertex_quadrics[a], boundary_q);
            add_mesh_simplify_quadric(vertex_quadrics[b], boundary_q);
        }
    }
}

detail::MeshSimplifyEdgeCandidate MeshSimplifier::make_edge_candidate(
  std::uint32_t a,
  std::uint32_t b)
{
    detail::MeshSimplifyQuadric q = vertex_quadrics[a];
    add_mesh_simplify_quadric(q, vertex_quadrics[b]);

    const bool a_is_boundary = boundary_vertices[a];
    const bool b_is_boundary = boundary_vertices[b];

    ml::vec3 position;
    if(a_is_boundary && b_is_boundary)
    {
        position = (vertex_positions[a] + vertex_positions[b]) * 0.5f;
    }
    else if(a_is_boundary)
    {
        position = vertex_positions[a];
    }
    else if(b_is_boundary)
    {
        position = vertex_positions[b];
    }
    else if(!solve_quadric_optimal_vertex(q, position))
    {
        position = (vertex_positions[a] + vertex_positions[b]) * 0.5f;
    }

    return {
      .cost = q.evaluate({position.x, position.y, position.z, 1.f}),
      .a = a,
      .b = b,
    };
}

MeshSimplifier::EdgeQueue MeshSimplifier::build_edge_queue()
{
    const auto edges = build_edges();
    rebuild_quadrics(edges);

    EdgeQueue queue;
    for(const auto& [edge_key, faces]: edges)
    {
        if(
          faces.size() != 1 /* boundary */
          && faces.size() != 2)
        {
            continue;
        }

        const auto [a, b] = unpack_edge_key(edge_key);
        if(simplify_settings.preserve_boundaries
           && (boundary_vertices[a] || boundary_vertices[b]))
        {
            continue;
        }

        queue.push(make_edge_candidate(a, b));
    }

    simplify_stats.queued_edges = queue.size();
    return queue;
}

swr::vector<std::size_t> MeshSimplifier::collect_affected_triangles(
  std::uint32_t a,
  std::uint32_t b)
{
    swr::vector<std::size_t> affected;
    swr::vector<bool> seen(triangles.size(), false);

    const auto append = [&](std::uint32_t vertex)
    {
        for(const std::size_t triangle_index: vertex_triangles[vertex])
        {
            if(!seen[triangle_index])
            {
                seen[triangle_index] = true;
                affected.push_back(triangle_index);
            }
        }
    };

    append(a);
    append(b);

    return affected;
}

bool MeshSimplifier::collapse_preserves_triangle(
  const std::array<std::uint32_t, 3>& roots,
  std::uint32_t removed,
  std::uint32_t kept,
  const ml::vec3& collapse_position,
  const swr::unordered_set<std::uint64_t>& existing,
  const swr::unordered_set<std::uint64_t>& affected_keys,
  swr::unordered_set<std::uint64_t>& proposed)
{
    const auto collapsed = collapsed_roots(roots, removed, kept);

    if(!is_valid_triangle(collapsed))
    {
        return true;
    }

    const auto position_of = [&](std::uint32_t root) -> ml::vec3
    {
        return root == kept ? collapse_position : vertex_positions[root];
    };

    const ml::vec3 old_normal = calculate_triangle_normal(roots, vertex_positions);

    // if(old_normal.length_squared() <= area_epsilon)
    // {
    //     return false;
    // }

    const ml::vec3 e0 =
      position_of(collapsed[1]) - position_of(collapsed[0]);
    const ml::vec3 e1 =
      position_of(collapsed[2]) - position_of(collapsed[0]);

    const ml::vec3 new_normal = e0.cross_product(e1);

    // if(new_normal.length_squared() <= area_epsilon)
    // {
    //     return false;
    // }

    if(old_normal.normalized().dot_product(new_normal.normalized()) <= 0.f)
    {
        return false;
    }

    const std::uint64_t key =
      triangle_key(collapsed[0], collapsed[1], collapsed[2]);

    if((existing.find(key) != existing.end()
        && affected_keys.find(key) == affected_keys.end())
       || proposed.find(key) != proposed.end())
    {
        return false;
    }

    proposed.insert(key);
    return true;
}

bool MeshSimplifier::can_collapse(
  std::uint32_t kept,
  std::uint32_t removed,
  const ml::vec3& collapse_position,
  const swr::vector<std::size_t>& affected)
{
    kept = root_of(kept);
    removed = root_of(removed);

    // Check for boundary pinching: If both vertices are boundary vertices,
    // they must only be collapsed if they form an actual boundary edge (1 face).
    // Collapsing an interior edge (2 faces) connecting two boundary vertices pinches/splits the hole!
    if(boundary_vertices[kept] && boundary_vertices[removed])
    {
        std::size_t shared_face_count = 0;
        for(const std::size_t triangle_index: affected)
        {
            if(!active_triangles[triangle_index])
            {
                continue;
            }

            const auto roots = triangle_roots(triangles[triangle_index]);
            if(!is_valid_triangle(roots))
            {
                continue;
            }

            const bool has_kept = (roots[0] == kept || roots[1] == kept || roots[2] == kept);
            const bool has_removed = (roots[0] == removed || roots[1] == removed || roots[2] == removed);

            if(has_kept && has_removed)
            {
                ++shared_face_count;
            }
        }

        if(shared_face_count > 1)
        {
            return false;
        }
    }

    swr::unordered_set<std::uint64_t> affected_keys;
    swr::unordered_set<std::uint64_t> proposed;
    affected_keys.reserve(affected.size());

    for(const std::size_t triangle_index: affected)
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        if(!is_valid_triangle(roots))
        {
            continue;
        }

        affected_keys.insert(triangle_key(roots[0], roots[1], roots[2]));
    }

    for(const std::size_t triangle_index: affected)
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        if(!is_valid_triangle(roots))
        {
            continue;
        }

        if(!collapse_preserves_triangle(
             roots,
             removed,
             kept,
             collapse_position,
             active_triangle_keys,
             affected_keys,
             proposed))
        {
            return false;
        }
    }

    return true;
}

void MeshSimplifier::push_vertex_edges_to_queue(
  std::uint32_t v,
  EdgeQueue& queue)
{
    v = root_of(v);
    swr::unordered_set<std::uint32_t> neighbors;

    for(const std::size_t triangle_index: vertex_triangles[v])
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        if(!is_valid_triangle(roots))
        {
            continue;
        }

        for(const auto root: roots)
        {
            if(root != v)
            {
                neighbors.insert(root);
            }
        }
    }

    for(const auto neighbor: neighbors)
    {
        if(simplify_settings.preserve_boundaries
           && (boundary_vertices[v] || boundary_vertices[neighbor]))
        {
            continue;
        }

        queue.push(make_edge_candidate(v, neighbor));
    }
}

void MeshSimplifier::simplify_until_target()
{
    std::size_t active_triangle_count = rebuild_adjacency();
    auto queue = build_edge_queue();

    while(active_triangle_count > simplify_stats.target_triangles
          && !queue.empty())
    {
        const auto candidate = queue.top();
        queue.pop();

        const std::uint32_t a = root_of(candidate.a);
        const std::uint32_t b = root_of(candidate.b);

        // Stale edge check: degenerate or already merged
        if(a == b)
        {
            continue;
        }

        if(simplify_settings.preserve_boundaries
           && (boundary_vertices[a] || boundary_vertices[b]))
        {
            ++simplify_stats.rejected_collapses;
            continue;
        }

        const auto quadric = detail::MeshSimplifyQuadric{
          .m = vertex_quadrics[a].m + vertex_quadrics[b].m};

        ml::vec3 collapse_position;
        const bool a_is_boundary = boundary_vertices[a];
        const bool b_is_boundary = boundary_vertices[b];

        if(a_is_boundary && b_is_boundary)
        {
            collapse_position = (vertex_positions[a] + vertex_positions[b]) * 0.5f;
        }
        else if(a_is_boundary)
        {
            collapse_position = vertex_positions[a];
        }
        else if(b_is_boundary)
        {
            collapse_position = vertex_positions[b];
        }
        else if(!solve_quadric_optimal_vertex(quadric, collapse_position))
        {
            const ml::vec3 pa = vertex_positions[a];
            const ml::vec3 pb = vertex_positions[b];
            const ml::vec3 pm = (pa + pb) * 0.5f;

            const float ea = quadric.evaluate({pa, 1.0f});
            const float eb = quadric.evaluate({pb, 1.0f});
            const float em = quadric.evaluate({pm, 1.0f});

            if(ea <= eb && ea <= em)
            {
                collapse_position = pa;
            }
            else if(eb <= em)
            {
                collapse_position = pb;
            }
            else
            {
                collapse_position = pm;
            }
        }

        const auto affected = collect_affected_triangles(a, b);

        if(!can_collapse(a, b, collapse_position, affected))
        {
            ++simplify_stats.rejected_collapses;
            continue;
        }

        const std::size_t removed_count = collapse_edge(a, b, collapse_position);
        active_triangle_count -= removed_count;

        // Push newly affected local edges into the queue rather than rebuilding whole queue
        push_vertex_edges_to_queue(a, queue);

        ++simplify_stats.accepted_collapses;
    }

    simplify_stats.output_triangles = active_triangle_count;
}

MeshData MeshSimplifier::build_output_mesh()
{
    MeshData lod{
      .primitive_type = source_mesh->primitive_type,
      .indices = {},
      .vertices = {},
      .normals = {},
      .texcoords = {}};

    std::vector<std::uint32_t> root_to_lod_vertex(
      source_mesh->vertices.size(),
      unmapped);

    std::vector<std::uint32_t> lod_vertex_roots;
    std::vector<ml::vec3> normal_sums;

    const auto lod_vertex_for_root = [&](std::uint32_t root)
    {
        root = root_of(root);

        std::uint32_t& mapped = root_to_lod_vertex[root];
        if(mapped == unmapped)
        {
            mapped = static_cast<std::uint32_t>(lod.vertices.size());

            lod.vertices.push_back({
              vertex_positions[root].x,
              vertex_positions[root].y,
              vertex_positions[root].z,
              1.f,
            });

            lod_vertex_roots.push_back(root);
            normal_sums.emplace_back();
        }

        return mapped;
    };

    for(std::size_t triangle_index = 0;
        triangle_index < triangles.size();
        ++triangle_index)
    {
        if(!active_triangles[triangle_index])
        {
            continue;
        }

        const auto roots = triangle_roots(triangles[triangle_index]);
        if(!is_valid_triangle(roots))
        {
            continue;
        }

        const ml::vec3 face_normal = calculate_triangle_normal(roots, vertex_positions);

        // if(face_normal.length_squared() <= area_epsilon)
        // {
        //     continue;
        // }

        const std::uint32_t i0 = lod_vertex_for_root(roots[0]);
        const std::uint32_t i1 = lod_vertex_for_root(roots[1]);
        const std::uint32_t i2 = lod_vertex_for_root(roots[2]);

        if(!is_valid_triangle({i0, i1, i2}))
        {
            continue;
        }

        lod.indices.push_back(i0);
        lod.indices.push_back(i1);
        lod.indices.push_back(i2);

        normal_sums[i0] += face_normal;
        normal_sums[i1] += face_normal;
        normal_sums[i2] += face_normal;
    }

    if(simplify_settings.recompute_normals)
    {
        lod.normals.reserve(lod.vertices.size());

        for(std::size_t i = 0; i < lod.vertices.size(); ++i)
        {
            const ml::vec3& n = normal_sums[i];

            if(n.length_squared() > area_epsilon)
            {
                const ml::vec3 unit = n.normalized();
                lod.normals.push_back({unit.x, unit.y, unit.z, 0.f});
            }
            else
            {
                const std::uint32_t root = lod_vertex_roots[i];

                lod.normals.push_back(
                  root < source_mesh->normals.size()
                    ? source_mesh->normals[root]
                    : ml::vec4{0.f, 0.f, 1.f, 0.f});
            }
        }
    }

    simplify_stats.output_triangles = lod.indices.size() / 3;
    return lod;
}

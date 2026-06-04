/**
 * Software Rasterizer Playground.
 *
 * Mesh simplifier implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <utility>

#include "mesh_simplifier.h"

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
      std::max<std::size_t>(
        1,
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
           || i0 == i1
           || i0 == i2
           || i1 == i2)
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

bool MeshSimplifier::is_valid_triangle(
  const std::array<std::uint32_t, 3>& roots)
{
    return roots[0] != roots[1]
           && roots[0] != roots[2]
           && roots[1] != roots[2];
}

std::uint64_t MeshSimplifier::triangle_key(
  std::uint32_t a,
  std::uint32_t b,
  std::uint32_t c)
{
    if(a > b)
        std::swap(a, b);
    if(b > c)
        std::swap(b, c);
    if(a > b)
        std::swap(a, b);

    return (static_cast<std::uint64_t>(a) << 42)
           | (static_cast<std::uint64_t>(b) << 21)
           | static_cast<std::uint64_t>(c);
}

MeshSimplifier::EdgeMap MeshSimplifier::build_edges()
{
    EdgeMap edges;

    const auto add_edge =
      [&](std::uint32_t a, std::uint32_t b, std::size_t triangle_index)
    {
        if(a > b)
        {
            std::swap(a, b);
        }

        edges[detail::make_mesh_simplify_edge_key(a, b)]
          .push_back(triangle_index);
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

        add_edge(roots[0], roots[1], triangle_index);
        add_edge(roots[1], roots[2], triangle_index);
        add_edge(roots[2], roots[0], triangle_index);
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
            const std::uint32_t a =
              static_cast<std::uint32_t>(edge_key >> 32u);
            const std::uint32_t b =
              static_cast<std::uint32_t>(edge_key & 0xffffffffu);

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

void MeshSimplifier::rebuild_quadrics()
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

        const ml::vec3 normal =
          detail::mesh_simplify_triangle_normal(roots, vertex_positions);

        if(normal.length_squared() <= area_epsilon)
        {
            continue;
        }

        const ml::vec3 n = normal.normalized();
        const float d = -n.dot_product(vertex_positions[roots[0]]);

        const auto q =
          detail::make_mesh_simplify_quadric({n.x, n.y, n.z, d});

        add_mesh_simplify_quadric(vertex_quadrics[roots[0]], q);
        add_mesh_simplify_quadric(vertex_quadrics[roots[1]], q);
        add_mesh_simplify_quadric(vertex_quadrics[roots[2]], q);
    }
}

detail::MeshSimplifyEdgeCandidate MeshSimplifier::make_edge_candidate(
  std::uint32_t a,
  std::uint32_t b)
{
    detail::MeshSimplifyQuadric q = vertex_quadrics[a];
    add_mesh_simplify_quadric(q, vertex_quadrics[b]);

    const ml::vec3 midpoint = (vertex_positions[a] + vertex_positions[b]) * 0.5f;
    ml::vec3 optimal = midpoint;

    const bool solved =
      detail::solve_mesh_simplify_optimal_position(q, optimal);

    const float midpoint_cost =
      detail::evaluate_mesh_simplify_quadric(
        q,
        {midpoint.x, midpoint.y, midpoint.z, 1.f});

    const float optimal_cost =
      solved
        ? detail::evaluate_mesh_simplify_quadric(
            q,
            {optimal.x, optimal.y, optimal.z, 1.f})
        : midpoint_cost;

    return {
      .cost = std::min(midpoint_cost, optimal_cost),
      .a = a,
      .b = b,
    };
}

MeshSimplifier::EdgeQueue MeshSimplifier::build_edge_queue()
{
    rebuild_quadrics();

    EdgeQueue queue;

    for(const auto& [edge_key, faces]: build_edges())
    {
        if(faces.size() != 2)
        {
            continue;
        }

        const std::uint32_t a =
          static_cast<std::uint32_t>(edge_key >> 32u);
        const std::uint32_t b =
          static_cast<std::uint32_t>(edge_key & 0xffffffffu);

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

std::vector<std::size_t> MeshSimplifier::collect_affected_triangles(
  std::uint32_t a,
  std::uint32_t b)
{
    std::vector<std::size_t> affected;
    std::vector<bool> seen(triangles.size(), false);

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

std::array<std::uint32_t, 3> MeshSimplifier::collapsed_roots(
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

bool MeshSimplifier::collapse_preserves_triangle(
  const std::array<std::uint32_t, 3>& roots,
  std::uint32_t removed,
  std::uint32_t kept,
  const ml::vec3& collapse_position,
  const std::unordered_set<std::uint64_t>& existing,
  const std::unordered_set<std::uint64_t>& affected_keys,
  std::unordered_set<std::uint64_t>& proposed)
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

    const ml::vec3 old_normal =
      detail::mesh_simplify_triangle_normal(roots, vertex_positions);

    if(old_normal.length_squared() <= area_epsilon)
    {
        return false;
    }

    const ml::vec3 e0 =
      position_of(collapsed[1]) - position_of(collapsed[0]);
    const ml::vec3 e1 =
      position_of(collapsed[2]) - position_of(collapsed[0]);

    const ml::vec3 new_normal = e0.cross_product(e1);

    if(new_normal.length_squared() <= area_epsilon)
    {
        return false;
    }

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
  const std::vector<std::size_t>& affected)
{
    std::unordered_set<std::uint64_t> affected_keys;
    std::unordered_set<std::uint64_t> proposed;
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

void MeshSimplifier::simplify_until_target()
{
    std::size_t active_triangle_count = rebuild_adjacency();
    auto queue = build_edge_queue();

    std::size_t attempts = 0;
    const std::size_t max_attempts = triangles.size() * 10;

    while(active_triangle_count > simplify_stats.target_triangles
          && !queue.empty()
          && attempts < max_attempts)
    {
        const auto candidate = queue.top();
        queue.pop();
        ++attempts;

        const std::uint32_t a = root_of(candidate.a);
        const std::uint32_t b = root_of(candidate.b);

        if(a == b
           || (simplify_settings.preserve_boundaries
               && (boundary_vertices[a] || boundary_vertices[b])))
        {
            ++simplify_stats.rejected_collapses;
            continue;
        }

        const ml::vec3 collapse_position =
          (vertex_positions[a] + vertex_positions[b]) * 0.5f;

        const auto affected = collect_affected_triangles(a, b);

        if(!can_collapse(a, b, collapse_position, affected))
        {
            ++simplify_stats.rejected_collapses;
            continue;
        }

        vertex_parents[b] = a;
        vertex_positions[a] = collapse_position;
        boundary_vertices[a] =
          boundary_vertices[a] || boundary_vertices[b];

        active_triangle_count = rebuild_adjacency();
        queue = build_edge_queue();

        ++simplify_stats.accepted_collapses;
    }

    simplify_stats.output_triangles = active_triangle_count;
}

MeshData MeshSimplifier::build_output_mesh()
{
    MeshData lod{
      .primitive_type = source_mesh->primitive_type,
    };

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

        const ml::vec3 face_normal =
          detail::mesh_simplify_triangle_normal(roots, vertex_positions);

        if(face_normal.length_squared() <= area_epsilon)
        {
            continue;
        }

        const std::uint32_t i0 = lod_vertex_for_root(roots[0]);
        const std::uint32_t i1 = lod_vertex_for_root(roots[1]);
        const std::uint32_t i2 = lod_vertex_for_root(roots[2]);

        if(i0 == i1 || i0 == i2 || i1 == i2)
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

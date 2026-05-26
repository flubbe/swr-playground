/**
 * Software Rasterizer Playground.
 *
 * A renderer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <chrono>
#include <array>
#include <utility>

#include "renderdevice.h"
#include "renderer.h"
#include "scene/scene.h"
#include "scene/static_mesh.h"
#include "viewport.h"

namespace
{

std::array<ml::vec4, 8> make_bounds_corners(
  const MeshBounds& bounds)
{
    const auto& min = bounds.min;
    const auto& max = bounds.max;

    return {{
      {min.x, min.y, min.z, 1.f},
      {max.x, min.y, min.z, 1.f},
      {min.x, max.y, min.z, 1.f},
      {max.x, max.y, min.z, 1.f},
      {min.x, min.y, max.z, 1.f},
      {max.x, min.y, max.z, 1.f},
      {min.x, max.y, max.z, 1.f},
      {max.x, max.y, max.z, 1.f},
    }};
}

template<typename PlaneFn>
bool all_corners_outside(
  const std::array<ml::vec4, 8>& clip_corners,
  PlaneFn plane)
{
    for(const auto& corner: clip_corners)
    {
        if(plane(corner) >= 0.f)
        {
            return false;
        }
    }

    return true;
}

bool bounds_intersect_frustum(
  const MeshBounds& bounds,
  const ml::mat4x4& clip_from_mesh)
{
    if(!bounds.valid)
    {
        return true;
    }

    const auto corners = make_bounds_corners(bounds);
    std::array<ml::vec4, 8> clip_corners{};
    for(std::size_t i = 0; i < corners.size(); ++i)
    {
        clip_corners[i] = clip_from_mesh * corners[i];
    }

    return !(all_corners_outside(
               clip_corners,
               [](const ml::vec4& corner)
               {
                   return corner.x + corner.w;
               })
             || all_corners_outside(
               clip_corners,
               [](const ml::vec4& corner)
               {
                   return -corner.x + corner.w;
               })
             || all_corners_outside(
               clip_corners,
               [](const ml::vec4& corner)
               {
                   return corner.y + corner.w;
               })
             || all_corners_outside(
               clip_corners,
               [](const ml::vec4& corner)
               {
                   return -corner.y + corner.w;
               })
             || all_corners_outside(
               clip_corners,
               [](const ml::vec4& corner)
               {
                   return corner.z + corner.w;
               })
             || all_corners_outside(
               clip_corners,
               [](const ml::vec4& corner)
               {
                   return -corner.z + corner.w;
               }));
}

}    // namespace

void Renderer::create_grid_mesh()
{
    const auto color_gray = ml::vec4{0.5, 0.5, 0.5, 1.0};
    auto* gray_shader = shader_cache.add<shader::ColorOnly>(color_gray);
    auto gray_material = device.create_material(*gray_shader);

    // FIXME Materials are released by the render device on shutdown.

    std::vector<ml::vec4> vb;
    std::vector<ml::vec4> nb;
    std::vector<std::uint32_t> ib;

    constexpr int half_extent = 20;
    constexpr float spacing = 2.f;

    const ml::vec4 up_normal{0.0f, 1.0f, 0.0f, 0.0f};

    for(int i = -half_extent; i <= half_extent; ++i)
    {
        const float p = static_cast<float>(i) * spacing;

        // Line parallel to X axis.
        {
            const std::uint32_t base = static_cast<std::uint32_t>(vb.size());

            vb.push_back({-half_extent * spacing, 0.0f, p, 1.0f});
            vb.push_back({half_extent * spacing, 0.0f, p, 1.0f});

            nb.push_back(up_normal);
            nb.push_back(up_normal);

            ib.push_back(base + 0);
            ib.push_back(base + 1);
        }

        // Line parallel to Z axis.
        {
            const std::uint32_t base = static_cast<std::uint32_t>(vb.size());

            vb.push_back({p, 0.0f, -half_extent * spacing, 1.0f});
            vb.push_back({p, 0.0f, half_extent * spacing, 1.0f});

            nb.push_back(up_normal);
            nb.push_back(up_normal);

            ib.push_back(base + 0);
            ib.push_back(base + 1);
        }
    }

    overlay_grid = {
      .mesh_handle = device.create_mesh(
        MeshData{
          .primitive_type = PrimitiveType::Lines,
          .indices = std::move(ib),
          .vertices = std::move(vb),
          .normals = std::move(nb),
        }),
      .material_handle = gray_material};
}

void Renderer::render_scene(
  const Scene& scene,
  const Camera& camera,
  const ViewportDisplaySettings& display_settings)
{
    device.bind_rasterizer_state({.wireframe = display_settings.wireframe,
                                  .cull_face = display_settings.cull_face});

    auto view = camera.get_transform();
    auto projection = camera.get_projection_matrix();
    auto light_dir = ml::matrices::translation(view.rows[0].w, view.rows[1].w, view.rows[2].w)
                     * scene.get_light().position;

    scene.for_each_object<StaticMesh>(
      [this, &display_settings, &projection, &view, &light_dir](
        const StaticMesh& static_mesh)
      {
        ++render_stats.static_meshes;

        if(!static_mesh.has_mesh_sections())
        {
            return;
        }

        auto obj_view = view * static_mesh.get_transform();
        auto obj_clip = projection * obj_view;

        for(const auto& section: static_mesh.get_mesh_sections())
        {
            const MeshBounds* bounds = device.get_mesh_bounds(section.mesh_handle);
            if(display_settings.cull_frustum
               && bounds != nullptr
               && !bounds_intersect_frustum(*bounds, obj_clip))
            {
                ++render_stats.mesh_sections_culled;
                continue;
            }

            device.bind_material(section.material_handle);
            device.bind_uniforms({.proj = projection,
                                  .view = obj_view,
                                  .light_dir = light_dir}

            );
            device.draw_mesh(section.mesh_handle);
            ++render_stats.mesh_sections_drawn;
        }
      });
}

void Renderer::render_grid(
  const Camera& camera)
{
    device.bind_rasterizer_state({
      .wireframe = false,
      .cull_face = false,
    });

    auto view = camera.get_transform();
    auto projection = camera.get_projection_matrix();

    device.bind_material(overlay_grid.material_handle);
    device.bind_uniforms({
      .proj = projection,
      .view = view,
      .light_dir = {},
    });

    device.draw_mesh(overlay_grid.mesh_handle);
}

void Renderer::render(
  const Scene& scene,
  const Viewport& viewport)
{
    auto render_start_time = std::chrono::steady_clock::now();
    render_stats = {};

    const auto& display_settings = viewport.get_display_settings();
    const auto& overlay_settings = viewport.get_overlay_settings();

    device.begin_frame();

    const Camera& camera = viewport.get_camera(scene);

    /*
     * scene rendering.
     */

    render_scene(
      scene,
      camera,
      display_settings);

    /*
     * viewport overlays.
     */

    if(overlay_settings.show_grid)
    {
        render_grid(
          camera);
    }

    device.end_frame();

    render_time = std::chrono::duration<float>(
                    std::chrono::steady_clock::now() - render_start_time)
                    .count();
}

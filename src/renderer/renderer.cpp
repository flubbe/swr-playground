/**
 * Software Rasterizer Playground.
 *
 * A renderer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

#include "assets/shaders/color_only.h"
#include "assets/shaders/color_flat.h"
#include "assets/shaders/color_smooth.h"
#include "assets/shaders/lit_smooth.h"
#include "assets/shaders/phong_smooth.h"
#include "assets/shaders/shadow_depth.h"
#include "assets/shaders/shadow_map_debug.h"
#include "assets/shaders/textured_floor.h"
#include "assets/shaders/textured_shiny_floor.h"
#include "containers/format.h"
#include "containers/vector.h"
#include "logging.h"
#include "material_manager.h"
#include "render_device.h"
#include "renderer.h"
#include "scene/directional_light.h"
#include "scene/spotlight.h"
#include "scene/scene.h"
#include "scene/static_mesh.h"
#include "viewport.h"

namespace
{

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"Renderer"};
    return logger;
}

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

ml::mat4x4 make_shadow_bias_matrix()
{
    return {
      {0.5f, 0.f, 0.f, 0.5f},
      {0.f, 0.5f, 0.f, 0.5f},
      {0.f, 0.f, 0.5f, 0.5f},
      {0.f, 0.f, 0.f, 1.f},
    };
}

std::array<ml::vec3, 16> make_benchmark_positions()
{
    constexpr float benchmark_distance = 40.f;
    constexpr float benchmark_height = 0.f;

    std::array<ml::vec3, 16> positions{};
    for(std::size_t index = 0; index < positions.size(); ++index)
    {
        const float yaw = static_cast<float>(index)
                          * (2.f * std::numbers::pi_v<float> / positions.size());
        positions[index] = {
          std::cos(yaw) * benchmark_distance,
          benchmark_height,
          std::sin(yaw) * benchmark_distance,
        };
    }

    return positions;
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

/**
 * Estimate the depth of a mesh using the mesh bounds' center.
 *
 * @param bounds Mesh bounds.
 * @param transform Mesh transformation matrix.
 * @returns Returns the estimated depth.
 */
float estimate_sort_depth(
  const MeshBounds& bounds,
  const ml::mat4x4& transform)
{
    const ml::vec4 local_center =
      bounds.valid
        ? ml::vec4{bounds.center, 1.f}
        : ml::vec4{0.f, 0.f, 0.f, 1.f};

    const ml::vec4 view_center = transform * local_center;
    return -view_center.z;
}

void record_selected_lod(
  RendererStats& stats,
  std::size_t lod_index)
{
    if(lod_index < stats.static_mesh_lods_selected.size())
    {
        ++stats.static_mesh_lods_selected[lod_index];
        return;
    }

    ++stats.static_mesh_lods_selected_overflow;
}

LightingUniforms collect_light_uniforms(
  const Scene& scene,
  const ml::mat4x4& camera_view)
{
    LightingUniforms uniforms{};

    std::size_t active_light_index = 0;
    for(auto& light: scene.objects_of<DirectionalLight>())
    {
        if(!light.enabled
           || active_light_index >= uniforms.directional_light_dirs.size())
        {
            continue;
        }

        uniforms.directional_light_dirs[active_light_index] =
          {(camera_view * ml::vec4{light.get_world_direction_to_light(), 0.f}).xyz().normalized(),
           light.brightness};

        ++active_light_index;
    }
    uniforms.directional_light_count = static_cast<int>(active_light_index);

    active_light_index = 0;
    for(auto& light: scene.objects_of<SpotLight>())
    {
        if(!light.enabled
           || active_light_index >= uniforms.spot_light_positions.size())
        {
            continue;
        }

        uniforms.spot_light_positions[active_light_index] =
          {(camera_view * ml::vec4{light.get_position(), 1.f}).xyz(),
           light.get_range()};
        uniforms.spot_light_directions[active_light_index] =
          {(camera_view * ml::vec4{light.get_world_spot_direction(), 0.f}).xyz().normalized(),
           light.brightness};
        uniforms.spot_light_params[active_light_index] =
          {std::cos(light.get_inner_cone_angle_radians()),
           std::cos(light.get_outer_cone_angle_radians()),
           0.f,
           0.f};
        uniforms.spot_light_colors[active_light_index] =
          light.get_color();

        ++active_light_index;
    }

    uniforms.spot_light_count = static_cast<int>(active_light_index);
    return uniforms;
}

std::optional<ShadowCamera> collect_shadow_camera(
  const Scene& scene)
{
    for(const auto& light: scene.objects_of<SpotLight>())
    {
        if(!light.enabled
           || !light.casts_shadows)
        {
            continue;
        }

        const float outer_angle = light.get_outer_cone_angle_radians();
        const float range = light.get_range();
        if(range <= 0.1f)
        {
            continue;
        }

        constexpr float near_plane = 0.5f;
        const ml::vec3 position = light.get_position();
        const ml::vec3 direction = light.get_world_spot_direction();

        // Expand shadow frustum FOV beyond the spotlight cone to avoid clipping
        // casters near the projection boundary in shadow-map UV space.
        const float shadow_fov =
          std::min(outer_angle * 4.f, ml::to_radians(170.f));

        return std::make_optional(
          ShadowCamera{
            .proj = ml::matrices::perspective_projection(
              1.f,
              shadow_fov,
              near_plane,
              range),
            .view = ml::matrices::look_at(
              position,
              position + direction,
              {0.f, 1.f, 0.f}),
            .light = &light});
    }

    return std::nullopt;
}

}    // namespace

void Renderer::register_shaders()
{
    // register all shaders.
    shader_factory.register_shader<shader::ColorFlat>();
    shader_factory.register_shader<shader::ColorSmooth>();
    shader_factory.register_shader<shader::LitSmooth>();
    shader_factory.register_shader<shader::PhongSmooth>();
    shader_factory.register_shader<shader::TexturedFloor>();
    shader_factory.register_shader<shader::TexturedShinyFloor>();

    auto shader_names = shader_factory.get_names();
    std::ranges::sort(shader_names);
    get_logger().logf("Registered shaders: {}", shader_names);
}

void Renderer::build_render_queue(
  const Scene& scene,
  const ViewportDisplaySettings& display_settings)
{
    for(const auto& static_mesh: scene.objects_of<StaticMesh>())
    {
        if(!static_mesh.is_visible())
        {
            continue;
        }

        ++render_stats.static_meshes;

        if(!static_mesh.has_mesh_sections())
        {
            continue;
        }

        const auto obj_transform = static_mesh.get_transform();
        const auto& obj_bounds = static_mesh.get_bounds();

        const auto obj_view = view * obj_transform;
        const auto obj_clip = projection * obj_view;

        if(display_settings.cull_frustum
           && obj_bounds.valid
           && !bounds_intersect_frustum(obj_bounds, obj_clip))
        {
            continue;
        }

        const ml::vec3 view_center =
          (obj_view * ml::vec4{obj_bounds.center, 1.f}).xyz();

        const float distance = -view_center.z;
        if(distance <= 0.0f)
        {
            continue;
        }

        const float obj_sort_depth =
          estimate_sort_depth(obj_bounds, obj_view);

        ml::mat4x4 shadow_clip_from_mesh = ml::mat4x4::identity();
        if(shadow_camera)
        {
            shadow_clip_from_mesh =
              make_shadow_bias_matrix()
              * shadow_camera->proj
              * shadow_camera->view
              * obj_transform;
        }

        const float scale =
          std::max({obj_transform.rows[0].xyz().length(),
                    obj_transform.rows[1].xyz().length(),
                    obj_transform.rows[2].xyz().length()});
        const float world_radius =
          obj_bounds.radius * scale;
        const float projected_radius_pixels =
          world_radius * projection.rows[1].y * device.get_height() * 0.5f / distance;
        const float projected_pixel_area =
          std::numbers::pi_v<float> * projected_radius_pixels * projected_radius_pixels;

        const std::size_t lod_index =
          display_settings.dynamic_lod
            ? static_mesh.select_lod(
                projected_pixel_area,
                display_settings.target_pixels_per_triangle)
            : 0;    // always choose base LOD when dynamic LOD is disabled.

        record_selected_lod(render_stats, lod_index);

        const auto& lod = static_mesh.get_lod(lod_index);
        for(const auto& section: lod.mesh_sections)
        {
            // TODO We could add bound checks for the mesh sections here.

            if(auto material = section.material.try_get();
               material.has_value())
            {
                render_queue.push_back({
                  .sort_depth = obj_sort_depth,
                  .mesh_handle = section.mesh_handle,
                  .material_handle = material.value(),
                  .color = section.color,
                  .view_from_mesh = obj_view,
                  .shadow_map = {
                    .enabled =
                      shadow_camera.has_value()
                      && static_mesh.receives_shadows
                      && shadow_map != 0,
                    .handle = shadow_map,
                    .clip_from_mesh = shadow_clip_from_mesh,
                    .depth_bias = 0.0008f,
                    .linear_filter = shadow_linear_filter,
                  },
                });
            }

            // update stats.
            ++render_stats.mesh_sections_drawn;
            render_stats.triangles_submitted += section.triangle_count;
        }
    }
}

void Renderer::sort_render_queue(
  const ViewportDisplaySettings& display_settings)
{
    if(display_settings.sort_meshes)
    {
        render_queue_sorter->sort(
          render_queue);
    }
}

void Renderer::execute_render_queue()
{
    for(const DrawSubmission& submission: render_queue)
    {
        device.bind_shadow_map(submission.shadow_map);
        device.bind_material(submission.material_handle);
        device.bind_camera_uniforms({
          .proj = projection,
          .view = submission.view_from_mesh,
        });
        device.bind_material_uniforms({
          .base_color = submission.color,
        });
        device.bind_shadow_uniforms({
          .enabled = submission.shadow_map.enabled,
          .clip_from_mesh = submission.shadow_map.clip_from_mesh,
          .params = {
            submission.shadow_map.depth_bias,
            static_cast<float>(shadow_pcf_mode),
            0.f,
            0.f,
          },
        });
        device.draw_mesh(submission.mesh_handle);
    }
}

void Renderer::begin_scene_pass(
  const Scene& scene,
  const Camera& camera,
  const ViewportDisplaySettings& display_settings)
{
    /*
     * Clear queues.
     */
    render_queue.clear();

    /*
     * Collect scene pass state.
     */
    view = camera.get_transform();
    projection = camera.get_projection_matrix();

    shadow_linear_filter =
      shadow_pcf_mode == ShadowPcfMode::ModernBilinear3x3
      || shadow_pcf_mode == ShadowPcfMode::Stochastic4Tap
      || shadow_pcf_mode == ShadowPcfMode::Stochastic5Tap
      || shadow_pcf_mode == ShadowPcfMode::Stochastic4TapStable
      || shadow_pcf_mode == ShadowPcfMode::Stochastic4TapInterleaved;

    /*
     * Set up rasterizer.
     */
    device.bind_rasterizer_state({.wireframe = display_settings.wireframe,
                                  .cull_face = display_settings.cull_face});
    device.bind_lighting_uniforms(
      collect_light_uniforms(scene, view));
}

void Renderer::end_scene_pass()
{
    device.clear_shadow_map();
}

bool Renderer::begin_shadow_pass()
{
    shadow_queue.clear();

    if(!shadow_camera.has_value())
    {
        return false;
    }

    ensure_shadow_map_resources();

    device.begin_shadow_map_pass(shadow_map);

    device.bind_material(shadow_material);
    device.bind_lighting_uniforms({});
    device.bind_material_uniforms({});
    device.bind_shadow_uniforms({});

    return true;
}

void Renderer::end_shadow_pass()
{
    device.end_shadow_map_pass();
}

void Renderer::build_shadow_queue(
  const Scene& scene)
{
    for(const auto& static_mesh: scene.objects_of<StaticMesh>())
    {
        if(!static_mesh.is_visible()
           || !static_mesh.casts_shadows
           || !static_mesh.has_mesh_sections())
        {
            continue;
        }

        const auto& lod = static_mesh.get_lod(0);
        for(const auto& section: lod.mesh_sections)
        {
            shadow_queue.push_back({
              .mesh_handle = section.mesh_handle,
              .light_view_from_mesh =
                shadow_camera->view * static_mesh.get_transform(),
            });
        }
    }
}

void Renderer::execute_shadow_queue()
{
    for(const ShadowCasterSubmission& submission: shadow_queue)
    {
        device.bind_camera_uniforms({
          .proj = shadow_camera->proj,
          .view = submission.light_view_from_mesh,
        });
        device.draw_mesh(submission.mesh_handle);
    }
}

void Renderer::begin_render(
  const Scene& scene)
{
    render_stats = {};
    render_start_time = std::chrono::steady_clock::now();

    shadow_camera = collect_shadow_camera(scene);

    device.begin_frame();
}

void Renderer::end_render()
{
    device.end_frame();

    render_time = std::chrono::duration<float>(
                    std::chrono::steady_clock::now() - render_start_time)
                    .count();
}

void Renderer::create_grid_mesh()
{
    release_grid_mesh();

    const auto color_gray = ml::vec4{0.5, 0.5, 0.5, 1.0};
    auto* gray_shader = shader_factory.get_or_create<shader::ColorOnly>();
    auto gray_material = device.create_material(
      Material{
        .shader_handle = device.create_shader(*gray_shader),
        .base_color_handle = {},
        .normal_map_handle = {}});

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

    overlay_grid = swr::make_unique<MeshSection>(
      MeshSection{
        .mesh_handle = device.create_mesh(
          MeshData{
            .primitive_type = PrimitiveType::Lines,
            .indices = std::move(ib),
            .vertices = std::move(vb),
            .normals = std::move(nb),
            .texcoords = {}}),
        .material = ResolvableMaterial{
          "GrayMaterial",
          gray_material},
        .color = color_gray});
}

void Renderer::release_grid_mesh()
{
    if(overlay_grid != nullptr
       && overlay_grid->mesh_handle)
    {
        device.delete_mesh(overlay_grid->mesh_handle);
        overlay_grid->mesh_handle = {};
    }

    overlay_grid.reset();

    if(grid_material != 0)
    {
        device.delete_material(grid_material);
        grid_material = {};
    }

    if(grid_shader != 0)
    {
        device.delete_shader(grid_shader);
        grid_shader = {};
    }
}

void Renderer::create_spotlight_depth_debug_mesh()
{
    release_spotlight_depth_debug_mesh();

    auto* debug_shadow_shader = shader_factory.get_or_create<shader::ShadowMapDebug>();
    shadow_debug_overlay_shader = device.create_shader(*debug_shadow_shader);
    shadow_debug_overlay_material = device.create_material(
      Material{
        .shader_handle = shadow_debug_overlay_shader,
        .base_color_handle = {},
        .normal_map_handle = {}});

    std::vector<ml::vec4> qvb;
    std::vector<ml::vec4> qnb;
    std::vector<ml::vec4> qtb;
    std::vector<std::uint32_t> qib;

    qvb.reserve(4);
    qnb.reserve(4);
    qtb.reserve(4);
    qib.reserve(6);

    qvb.push_back({-1.f, -1.f, 0.f, 1.f});
    qvb.push_back({1.f, -1.f, 0.f, 1.f});
    qvb.push_back({1.f, 1.f, 0.f, 1.f});
    qvb.push_back({-1.f, 1.f, 0.f, 1.f});

    qnb.push_back({0.f, 0.f, 1.f, 0.f});
    qnb.push_back({0.f, 0.f, 1.f, 0.f});
    qnb.push_back({0.f, 0.f, 1.f, 0.f});
    qnb.push_back({0.f, 0.f, 1.f, 0.f});

    qtb.push_back({0.f, 0.f, 0.f, 0.f});
    qtb.push_back({1.f, 0.f, 0.f, 0.f});
    qtb.push_back({1.f, 1.f, 0.f, 0.f});
    qtb.push_back({0.f, 1.f, 0.f, 0.f});

    qib.push_back(0);
    qib.push_back(1);
    qib.push_back(2);
    qib.push_back(0);
    qib.push_back(2);
    qib.push_back(3);

    overlay_spotlight_depth = swr::make_unique<MeshSection>(
      MeshSection{
        .mesh_handle = device.create_mesh(
          MeshData{
            .primitive_type = PrimitiveType::Triangles,
            .indices = std::move(qib),
            .vertices = std::move(qvb),
            .normals = std::move(qnb),
            .texcoords = std::move(qtb),
          }),
        .material = ResolvableMaterial{
          "SpotlightDepthDebug",
          shadow_debug_overlay_material},
        .color = {1.f, 1.f, 1.f, 1.f},
      });
}

void Renderer::release_spotlight_depth_debug_mesh()
{
    if(overlay_spotlight_depth != nullptr
       && overlay_spotlight_depth->mesh_handle)
    {
        device.delete_mesh(overlay_spotlight_depth->mesh_handle);
        overlay_spotlight_depth->mesh_handle = {};
    }

    overlay_spotlight_depth.reset();

    if(shadow_debug_overlay_material != 0)
    {
        device.delete_material(shadow_debug_overlay_material);
        shadow_debug_overlay_material = {};
    }

    if(shadow_debug_overlay_shader != 0)
    {
        device.delete_shader(shadow_debug_overlay_shader);
        shadow_debug_overlay_shader = {};
    }
}

Renderer::~Renderer()
{
    release_spotlight_depth_debug_mesh();
    release_grid_mesh();
    release_shadow_map_resources();
}

void Renderer::ensure_shadow_map_resources()
{
    if(shadow_map != 0
       && shadow_material != 0)
    {
        return;
    }

    /*
     * Release previous resources.
     */
    release_shadow_map_resources();

    /*
     * Create new resources.
     */
    shadow_map = device.create_shadow_map(
      shadow_map_resolution,
      shadow_map_resolution);

    auto* shadow_shader = shader_factory.get_or_create<shader::ShadowDepth>();
    shadow_material = device.create_material(
      Material{
        .shader_handle = device.create_shader(*shadow_shader),
        .base_color_handle = {},
        .normal_map_handle = {}});
}

void Renderer::release_shadow_map_resources()
{
    if(shadow_map != 0)
    {
        device.delete_shadow_map(shadow_map);
        shadow_map = {};
    }

    if(shadow_material != 0)
    {
        device.delete_material(shadow_material);
        shadow_material = {};
    }
}

void Renderer::render_shadow_map(
  const Scene& scene)
{
    if(!begin_shadow_pass())
    {
        return;
    }

    build_shadow_queue(scene);
    execute_shadow_queue();
    end_shadow_pass();
}

void Renderer::render_scene(
  const Scene& scene,
  const Camera& camera,
  const ViewportDisplaySettings& display_settings)
{
    begin_scene_pass(
      scene,
      camera,
      display_settings);
    build_render_queue(
      scene,
      display_settings);
    sort_render_queue(display_settings);
    execute_render_queue();
    end_scene_pass();
}

void Renderer::render_grid(
  const Camera& camera)
{
    if(overlay_grid == nullptr)
    {
        return;
    }

    std::optional<MaterialHandle> material = overlay_grid->material.try_get();
    if(!material.has_value())
    {
        return;
    }

    device.bind_rasterizer_state({
      .wireframe = false,
      .cull_face = false,
    });

    auto view = camera.get_transform();
    auto projection = camera.get_projection_matrix();

    device.bind_material(material.value());
    device.bind_camera_uniforms({
      .proj = projection,
      .view = view,
    });
    device.bind_lighting_uniforms({});
    device.bind_material_uniforms({
      .base_color = overlay_grid->color,
    });
    device.bind_shadow_uniforms({});

    device.draw_mesh(overlay_grid->mesh_handle);
}

void Renderer::render_spotlight_depth_debug()
{
    if(shadow_map == 0)
    {
        return;
    }

    std::optional<MaterialHandle> material = overlay_spotlight_depth->material.try_get();
    if(!material.has_value())
    {
        return;
    }

    device.bind_rasterizer_state({
      .wireframe = false,
      .cull_face = false,
    });

    device.bind_shadow_map({
      .enabled = true,
      .handle = shadow_map,
      .clip_from_mesh = ml::mat4x4::identity(),
      .depth_bias = 0.f,
      .linear_filter = false,
    });
    device.bind_material(material.value());
    device.bind_camera_uniforms({
      .proj = ml::mat4x4::identity(),
      .view = ml::mat4x4::identity(),
    });
    device.bind_lighting_uniforms({});
    device.bind_material_uniforms({
      .base_color = {1.f, 1.f, 1.f, 1.f},
    });
    device.bind_shadow_uniforms({
      .enabled = true,
      .clip_from_mesh = ml::mat4x4::identity(),
      .params = {0.f, static_cast<float>(ShadowPcfMode::Off), 0.f, 0.f},
    });
    device.draw_mesh(overlay_spotlight_depth->mesh_handle);
    device.clear_shadow_map();
}

void Renderer::render(
  const Scene& scene,
  const Viewport& viewport)
{
    const auto& display_settings = viewport.get_display_settings();
    const auto& overlay_settings = viewport.get_overlay_settings();

    const Camera& camera = viewport.get_camera(scene);

    // Process pending resources.
    process_pending_resources();

    begin_render(scene);

    /*
     * Scene rendering.
     */

    render_shadow_map(scene);

    if(display_settings.debug_spotlight_depth
       && shadow_camera.has_value())
    {
        render_spotlight_depth_debug();
    }
    else
    {
        render_scene(
          scene,
          camera,
          display_settings);
    }

    /*
     * Viewport overlays.
     */

    if(!display_settings.debug_spotlight_depth
       && overlay_settings.show_grid)
    {
        render_grid(
          camera);
    }

    end_render();
}

void Renderer::start_sorting_benchmark(
  Scene& scene,
  Viewport& viewport,
  std::size_t iterations)
{
    if(benchmark_state.active)
    {
        return;
    }

    benchmark_state.active = true;
    benchmark_state.sorted_phase = true;
    benchmark_state.target_iterations = iterations;
    benchmark_state.current_iteration = 0;
    benchmark_state.total_time_sorted = 0.f;
    benchmark_state.total_time_unsorted = 0.f;
    benchmark_state.camera_positions = make_benchmark_positions();
    benchmark_state.saved_camera_transform = viewport.get_camera(scene).get_transform();
}

void Renderer::start_comparative_benchmark(
  Scene& scene,
  Viewport& viewport,
  std::size_t iterations)
{
    if(comparative_state.active)
    {
        return;
    }

    comparative_state.modes = {SortMode::FullSort, SortMode::BinSort};
    comparative_state.results.assign(comparative_state.modes.size(), {});
    comparative_state.current_mode_index = 0;
    comparative_state.saved_sort_mode = get_sort_mode();
    comparative_state.iterations_per_mode = iterations;
    comparative_state.active = true;

    // start first mode
    set_sort_mode(comparative_state.modes[comparative_state.current_mode_index]);
    start_sorting_benchmark(scene, viewport, iterations);
}

void Renderer::update_sorting_benchmark(
  Scene& scene,
  Viewport& viewport)
{
    if(!benchmark_state.active)
    {
        return;
    }

    const ViewportDisplaySettings display_settings = viewport.get_display_settings();
    const auto overlay_settings = viewport.get_overlay_settings();
    const ml::vec3 target{0.f, 0.f, 0.f};
    const ml::vec3 up{0.f, 1.f, 0.f};

    if(benchmark_state.current_iteration >= benchmark_state.target_iterations)
    {
        if(benchmark_state.sorted_phase)
        {
            benchmark_state.sorted_phase = false;
            benchmark_state.current_iteration = 0;
            return;
        }

        Camera& viewport_camera = viewport.get_camera(scene);
        viewport_camera.set_transform(benchmark_state.saved_camera_transform);
        viewport_camera.update_projection_matrix(viewport.get_aspect_ratio());

        benchmark_state.active = false;
        benchmark_results = {
          .time_with_sorting = benchmark_state.total_time_sorted,
          .time_without_sorting = benchmark_state.total_time_unsorted,
          .iterations = benchmark_state.target_iterations,
        };

        if(comparative_state.active)
        {
            // record results for current mode
            if(comparative_state.current_mode_index < comparative_state.results.size())
            {
                comparative_state.results[comparative_state.current_mode_index] = benchmark_results;
            }
            comparative_state.current_mode_index += 1;
            if(comparative_state.current_mode_index < comparative_state.modes.size())
            {
                set_sort_mode(comparative_state.modes[comparative_state.current_mode_index]);
                start_sorting_benchmark(scene, viewport, comparative_state.iterations_per_mode);
                return;
            }
            // finished comparative run
            comparative_state.active = false;
            set_sort_mode(comparative_state.saved_sort_mode);
        }

        return;
    }

    const auto camera_position =
      benchmark_state.camera_positions[benchmark_state.current_iteration
                                       % benchmark_state.camera_positions.size()];

    Camera& benchmark_camera = viewport.get_camera(scene);
    benchmark_camera.set_transform(
      ml::matrices::look_at(camera_position, target, up));
    benchmark_camera.update_projection_matrix(viewport.get_aspect_ratio());

    const auto saved_render_stats = render_stats;
    const float saved_render_time = render_time;

    ViewportDisplaySettings benchmark_display_settings = display_settings;
    benchmark_display_settings.sort_meshes = benchmark_state.sorted_phase;

    auto start_time = std::chrono::steady_clock::now();

    device.begin_frame();
    render_scene(scene, benchmark_camera, benchmark_display_settings);
    if(overlay_settings.show_grid)
    {
        render_grid(benchmark_camera);
    }
    device.end_frame();

    const float elapsed = std::chrono::duration<float>(
                            std::chrono::steady_clock::now() - start_time)
                            .count();

    if(benchmark_state.sorted_phase)
    {
        benchmark_state.total_time_sorted += elapsed;
    }
    else
    {
        benchmark_state.total_time_unsorted += elapsed;
    }

    render_stats = saved_render_stats;
    render_time = saved_render_time;

    benchmark_state.current_iteration += 1;
    if(benchmark_state.current_iteration >= benchmark_state.target_iterations
       && !benchmark_state.sorted_phase)
    {
        Camera& viewport_camera = viewport.get_camera(scene);
        viewport_camera.set_transform(benchmark_state.saved_camera_transform);
        viewport_camera.update_projection_matrix(viewport.get_aspect_ratio());

        benchmark_state.active = false;
        benchmark_results = {
          .time_with_sorting = benchmark_state.total_time_sorted,
          .time_without_sorting = benchmark_state.total_time_unsorted,
          .iterations = benchmark_state.target_iterations,
        };

        if(comparative_state.active)
        {
            if(comparative_state.current_mode_index < comparative_state.results.size())
            {
                comparative_state.results[comparative_state.current_mode_index] = benchmark_results;
            }
            comparative_state.current_mode_index += 1;
            if(comparative_state.current_mode_index < comparative_state.modes.size())
            {
                set_sort_mode(comparative_state.modes[comparative_state.current_mode_index]);
                start_sorting_benchmark(scene, viewport, comparative_state.iterations_per_mode);
            }
            else
            {
                comparative_state.active = false;
                set_sort_mode(comparative_state.saved_sort_mode);
            }
        }
    }
}

void Renderer::process_pending_resources()
{
    using namespace std::literals;

    // TODO Could make this subject to a time budget.
    pending_materials.for_each(
      [](swr::shared_ptr<MaterialEntry>& entry)
      {
          if(entry->resources.future.wait_for(0ms) == std::future_status::ready)
          {
              entry->finalize();
              logging::logf(
                "Finalized material '{}'.",
                entry->name);

              return true;    // remove entry.
          }
          return false;    // keep entry for next time.
      });
}
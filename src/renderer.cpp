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
#include <utility>
#include <vector>

#include "renderdevice.h"
#include "renderer.h"
#include "shader.h"
#include "scene/directionallight.h"
#include "scene/spotlight.h"
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

ml::mat4x4 make_camera_view_matrix(
  const ml::vec3& eye,
  const ml::vec3& target,
  const ml::vec3& up)
{
    return ml::matrices::look_at(eye, target, up);
}

std::array<ml::vec3, 16> make_benchmark_positions()
{
    constexpr float benchmark_distance = 40.f;
    constexpr float benchmark_height = 0.f;

    std::array<ml::vec3, 16> positions{};
    for(std::size_t index = 0; index < positions.size(); ++index)
    {
        const float yaw = static_cast<float>(index)
                          * (2.f * static_cast<float>(M_PI) / positions.size());
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

float estimate_screen_height_fraction(
  const MeshBounds& bounds,
  const ml::mat4x4& clip_from_mesh)
{
    if(!bounds.valid)
    {
        return 1.f;
    }

    constexpr float w_epsilon = 0.0001f;
    float min_y = std::numeric_limits<float>::max();
    float max_y = std::numeric_limits<float>::lowest();

    const auto corners = make_bounds_corners(bounds);
    for(const auto& corner: corners)
    {
        const ml::vec4 clip = clip_from_mesh * corner;
        if(clip.w <= w_epsilon)
        {
            return 1.f;
        }

        const float ndc_y = clip.y / clip.w;
        min_y = std::min(min_y, ndc_y);
        max_y = std::max(max_y, ndc_y);
    }

    return std::clamp((max_y - min_y) * 0.5f, 0.f, 1.f);
}

float estimate_sort_depth(
  const MeshBounds& bounds,
  const ml::mat4x4& view_from_mesh)
{
    ml::vec4 local_center{0.f, 0.f, 0.f, 1.f};
    if(bounds.valid)
    {
        local_center = {
          (bounds.min.x + bounds.max.x) * 0.5f,
          (bounds.min.y + bounds.max.y) * 0.5f,
          (bounds.min.z + bounds.max.z) * 0.5f,
          1.f,
        };
    }

    // In view space, visible geometry is typically in negative Z.
    // Sorting by -z yields near-to-far submission.
    const ml::vec4 view_center = view_from_mesh * local_center;
    return -view_center.z;
}

struct DrawSubmission
{
    float sort_depth{0.f};
    std::uint32_t mesh_handle{0};
    std::uint32_t material_handle{0};
    ml::vec4 color{1.f, 1.f, 1.f, 1.f};
    ml::mat4x4 view_from_mesh;
    ShadowMapBinding shadow_map;
};

struct ShadowCasterSubmission
{
    std::uint32_t mesh_handle{0};
    ml::mat4x4 light_view_from_mesh;
};

struct ShadowCamera
{
    ml::mat4x4 proj{ml::mat4x4::identity()};
    ml::mat4x4 view{ml::mat4x4::identity()};
    const SpotLight* light{nullptr};
};

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
    const auto directional_lights = scene.get_directional_lights();

    std::size_t active_light_index = 0;
    for(const DirectionalLight* light: directional_lights)
    {
        if(light == nullptr
           || !light->enabled
           || active_light_index >= uniforms.directional_light_dirs.size())
        {
            continue;
        }

        const ml::vec3 world_dir =
          light->get_world_direction_to_light();
        uniforms.directional_light_dirs[active_light_index] =
          ml::vec4(
            (camera_view * ml::vec4(world_dir, 0.f)).xyz().normalized(),
            light->brightness);
        ++active_light_index;
    }

    uniforms.directional_light_count = static_cast<int>(active_light_index);

    const auto spot_lights = scene.get_spot_lights();
    active_light_index = 0;
    for(const SpotLight* light: spot_lights)
    {
        if(light == nullptr
           || !light->enabled
           || active_light_index >= uniforms.spot_light_positions.size())
        {
            continue;
        }

        const ml::vec3 world_position = light->get_position();
        const ml::vec3 world_direction = light->get_world_spot_direction();
        uniforms.spot_light_positions[active_light_index] =
          ml::vec4(
            (camera_view * ml::vec4(world_position, 1.f)).xyz(),
            light->get_range());
        uniforms.spot_light_directions[active_light_index] =
          ml::vec4(
            (camera_view * ml::vec4(world_direction, 0.f)).xyz().normalized(),
            light->brightness);
        uniforms.spot_light_params[active_light_index] =
          ml::vec4(
            static_cast<float>(std::cos(light->get_inner_cone_angle_radians())),
            static_cast<float>(std::cos(light->get_outer_cone_angle_radians())),
            0.f,
            0.f);
        uniforms.spot_light_colors[active_light_index] =
          light->get_color();
        ++active_light_index;
    }

    uniforms.spot_light_count = static_cast<int>(active_light_index);
    return uniforms;
}

std::optional<ShadowCamera> collect_shadow_camera(
  const Scene& scene)
{
    const auto spot_lights = scene.get_spot_lights();
    for(const SpotLight* light: spot_lights)
    {
        if(light == nullptr
           || !light->enabled
           || !light->casts_shadows)
        {
            continue;
        }

        const float outer_angle = light->get_outer_cone_angle_radians();
        const float range = light->get_range();
        if(range <= 0.1f)
        {
            continue;
        }

        constexpr float near_plane = 0.5f;
        const ml::vec3 position = light->get_position();
        const ml::vec3 direction = light->get_world_spot_direction();

        // Expand shadow frustum FOV beyond the spotlight cone to avoid clipping
        // casters near the projection boundary in shadow-map UV space.
        const float shadow_fov =
          std::min(outer_angle * 4.f, ml::to_radians(170.f));

        return ShadowCamera{
          .proj = ml::matrices::perspective_projection(
            1.f,
            shadow_fov,
            near_plane,
            range),
          .view = make_camera_view_matrix(
            position,
            position + direction,
            {0.f, 1.f, 0.f}),
          .light = light,
        };
    }

    return std::nullopt;
}

void bin_submissions_by_depth(
  std::vector<DrawSubmission>& submissions,
  float near_depth,
  float far_depth,
  std::size_t bin_count)
{
    if(bin_count == 0)
        bin_count = 1;

    const float bin_size = (far_depth - near_depth) / static_cast<float>(bin_count);

    std::vector<std::vector<DrawSubmission>> bins;
    bins.resize(bin_count);

    // Distribute submissions into bins
    for(const auto& submission: submissions)
    {
        int bin_index = 0;
        if(bin_size > 0.f)
        {
            bin_index = static_cast<int>((submission.sort_depth - near_depth) / bin_size);
        }
        bin_index = std::min(std::max(bin_index, 0), static_cast<int>(bin_count) - 1);
        bins[bin_index].push_back(submission);
    }

    // Reconstruct submissions from bins (front to back)
    submissions.clear();
    for(const auto& bin: bins)
    {
        for(const auto& submission: bin)
        {
            submissions.push_back(submission);
        }
    }
}

}    // namespace

void Renderer::create_grid_mesh()
{
    release_grid_mesh();

    const auto color_gray = ml::vec4{0.5, 0.5, 0.5, 1.0};
    auto* gray_shader = shader_cache.get_or_create<shader::ColorOnly>();
    auto gray_material = device.create_material(*gray_shader);

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
      .material_handle = gray_material,
      .color = color_gray};
}

void Renderer::release_grid_mesh()
{
    if(overlay_grid.mesh_handle != 0)
    {
        device.delete_mesh(overlay_grid.mesh_handle);
        overlay_grid.mesh_handle = 0;
    }
    if(overlay_grid.material_handle != 0)
    {
        device.delete_material(overlay_grid.material_handle, false);
        overlay_grid.material_handle = 0;
    }
}

void Renderer::create_spotlight_depth_debug_mesh()
{
    release_spotlight_depth_debug_mesh();

    auto* debug_shadow_shader = shader_cache.get_or_create<shader::ShadowMapDebug>();
    const auto debug_shadow_material = device.create_material(*debug_shadow_shader);

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

    overlay_spotlight_depth = {
      .mesh_handle = device.create_mesh(
        MeshData{
          .primitive_type = PrimitiveType::Triangles,
          .indices = std::move(qib),
          .vertices = std::move(qvb),
          .normals = std::move(qnb),
          .texcoords = std::move(qtb),
        }),
      .material_handle = debug_shadow_material,
      .color = {1.f, 1.f, 1.f, 1.f},
    };
}

void Renderer::release_spotlight_depth_debug_mesh()
{
    if(overlay_spotlight_depth.mesh_handle != 0)
    {
        device.delete_mesh(overlay_spotlight_depth.mesh_handle);
        overlay_spotlight_depth.mesh_handle = 0;
    }
    if(overlay_spotlight_depth.material_handle != 0)
    {
        device.delete_material(overlay_spotlight_depth.material_handle, false);
        overlay_spotlight_depth.material_handle = 0;
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
    if(shadow_map_texture != 0
       && shadow_map_framebuffer != 0
       && shadow_map_depth_renderbuffer != 0)
    {
        return;
    }

    release_shadow_map_resources();

    shadow_map_texture = device.create_empty_texture(
      shadow_map_resolution,
      shadow_map_resolution);
    swr::ActiveTexture(shader::shadow_map_sampler_unit);
    swr::BindTexture(
      swr::texture_target::texture_2d,
      shadow_map_texture);
    swr::SetTextureWrapMode(
      shadow_map_texture,
      swr::wrap_mode::clamp_to_edge,
      swr::wrap_mode::clamp_to_edge);
    swr::SetTextureMinificationFilter(swr::texture_filter::linear);
    swr::SetTextureMagnificationFilter(swr::texture_filter::linear);
    swr::BindTexture(swr::texture_target::texture_2d, 0);

    shadow_map_framebuffer = swr::CreateFramebufferObject();
    shadow_map_depth_renderbuffer = swr::CreateDepthRenderbuffer(
      shadow_map_resolution,
      shadow_map_resolution);

    swr::FramebufferTexture(
      shadow_map_framebuffer,
      swr::framebuffer_attachment::color_attachment_0,
      shadow_map_texture,
      0);
    swr::FramebufferRenderbuffer(
      shadow_map_framebuffer,
      swr::framebuffer_attachment::depth_attachment,
      shadow_map_depth_renderbuffer);
}

void Renderer::release_shadow_map_resources()
{
    if(shadow_map_depth_renderbuffer != 0)
    {
        swr::ReleaseDepthRenderbuffer(shadow_map_depth_renderbuffer);
        shadow_map_depth_renderbuffer = 0;
    }
    if(shadow_map_framebuffer != 0)
    {
        swr::ReleaseFramebufferObject(shadow_map_framebuffer);
        shadow_map_framebuffer = 0;
    }
    if(shadow_map_texture != 0)
    {
        device.delete_texture(shadow_map_texture);
        shadow_map_texture = 0;
    }
}

void Renderer::render_shadow_map(const Scene& scene)
{
    const auto shadow_camera = collect_shadow_camera(scene);
    if(!shadow_camera.has_value())
    {
        return;
    }

    ensure_shadow_map_resources();

    auto* shadow_shader = shader_cache.get_or_create<shader::ShadowDepth>();
    const std::uint32_t shadow_material = device.create_material(*shadow_shader);

    std::vector<ShadowCasterSubmission> submissions;
    scene.for_each_object<StaticMesh>(
      [&shadow_camera, &submissions](const StaticMesh& static_mesh)
      {
          if(!static_mesh.is_visible()
             || !static_mesh.casts_shadows
             || !static_mesh.has_mesh_sections())
          {
              return;
          }

          const auto& lod = static_mesh.get_lod(static_mesh.select_lod(1.f));
          for(const auto& section: lod.mesh_sections)
          {
              submissions.push_back({
                .mesh_handle = section.mesh_handle,
                .light_view_from_mesh =
                  shadow_camera->view * static_mesh.get_transform(),
              });
          }
      });

    swr::BindFramebufferObject(
      swr::framebuffer_target::draw,
      shadow_map_framebuffer);
    swr::SetViewport(0, 0, shadow_map_resolution, shadow_map_resolution);
    swr::SetClearColor(1.f, 1.f, 1.f, 1.f);
    swr::SetClearDepth(1.f);
    swr::ClearColorBuffer();
    swr::ClearDepthBuffer();
    swr::SetState(swr::state::depth_test, true);
    swr::SetState(swr::state::depth_write, true);
    swr::SetState(swr::state::texture, false);
    swr::SetState(swr::state::cull_face, true);
    swr::SetState(swr::state::polygon_offset_fill, true);
    swr::PolygonOffset(1.5f, 2.f);
    swr::SetPolygonMode(swr::polygon_mode::fill);

    device.bind_material(shadow_material);
    device.bind_lighting_uniforms({});
    device.bind_material_uniforms({});
    device.bind_shadow_uniforms({});
    for(const ShadowCasterSubmission& submission: submissions)
    {
        device.bind_camera_uniforms({
          .proj = shadow_camera->proj,
          .view = submission.light_view_from_mesh,
        });
        device.draw_mesh(submission.mesh_handle);
    }

    swr::SetState(swr::state::polygon_offset_fill, false);
    swr::BindFramebufferObject(swr::framebuffer_target::draw, 0);
    swr::SetViewport(0, 0, device.get_width(), device.get_height());
    swr::SetClearColor(0.f, 0.f, 0.f, 1.f);

    device.delete_material(shadow_material, false);
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
    const LightingUniforms lighting_uniforms = collect_light_uniforms(scene, view);
    const auto shadow_camera = collect_shadow_camera(scene);

    std::vector<DrawSubmission> submissions;

    scene.for_each_object<StaticMesh>(
      [this, &display_settings, &projection, &view, &submissions, &shadow_camera](
        const StaticMesh& static_mesh)
      {
          if(!static_mesh.is_visible())
          {
              return;
          }

          ++render_stats.static_meshes;

          if(!static_mesh.has_mesh_sections())
          {
              return;
          }

          const auto obj_view = view * static_mesh.get_transform();
          const auto obj_clip = projection * obj_view;
          const ml::mat4x4 shadow_clip_from_mesh =
            shadow_camera.has_value()
              ? shadow_camera->proj
                  * shadow_camera->view
                  * static_mesh.get_transform()
              : ml::mat4x4::identity();
          const float obj_sort_depth =
            estimate_sort_depth(static_mesh.get_bounds(), obj_view);

          const MeshBounds& mesh_bounds = static_mesh.get_bounds();
          if(display_settings.cull_frustum
             && mesh_bounds.valid
             && !bounds_intersect_frustum(mesh_bounds, obj_clip))
          {
              const std::size_t base_lod_index = static_mesh.select_lod(1.f);
              const auto& culled_lod = static_mesh.get_lod(base_lod_index);
              render_stats.mesh_sections_culled += culled_lod.mesh_sections.size();
              for(const auto& section: culled_lod.mesh_sections)
              {
                  render_stats.triangles_frustum_culled +=
                    device.get_mesh_triangle_count(section.mesh_handle);
              }
              return;
          }

          const float screen_height_fraction =
            estimate_screen_height_fraction(mesh_bounds, obj_clip);
          const std::size_t lod_index =
            display_settings.dynamic_lod
              ? static_mesh.select_lod(screen_height_fraction)
              : static_mesh.select_lod(1.f);
          record_selected_lod(render_stats, lod_index);

          const auto& lod = static_mesh.get_lod(lod_index);
          for(const auto& section: lod.mesh_sections)
          {
              const MeshBounds* bounds = device.get_mesh_bounds(section.mesh_handle);
              if(display_settings.cull_frustum
                 && bounds != nullptr
                 && !bounds_intersect_frustum(*bounds, obj_clip))
              {
                  ++render_stats.mesh_sections_culled;
                  render_stats.triangles_frustum_culled +=
                    device.get_mesh_triangle_count(section.mesh_handle);
                  continue;
              }

              submissions.push_back({
                .sort_depth = obj_sort_depth,
                .mesh_handle = section.mesh_handle,
                .material_handle = section.material_handle,
                .color = section.color,
                .view_from_mesh = obj_view,
                .shadow_map = {
                  .enabled =
                    shadow_camera.has_value()
                    && static_mesh.receives_shadows
                    && shadow_map_texture != 0,
                  .texture_handle = shadow_map_texture,
                  .clip_from_mesh = shadow_clip_from_mesh,
                  .depth_bias = 0.0008f,
                },
              });
              ++render_stats.mesh_sections_drawn;
              render_stats.triangles_submitted +=
                device.get_mesh_triangle_count(section.mesh_handle);
          }
      });

    if(display_settings.sort_meshes)
    {
        if(sort_mode == SortMode::FullSort)
        {
            std::stable_sort(
              submissions.begin(),
              submissions.end(),
              [](const DrawSubmission& lhs, const DrawSubmission& rhs)
              {
                  return lhs.sort_depth < rhs.sort_depth;
              });
        }
        else if(sort_mode == SortMode::BinSort && !submissions.empty())
        {
            // Find depth range for binning
            float min_depth = submissions[0].sort_depth;
            float max_depth = submissions[0].sort_depth;
            for(const auto& submission: submissions)
            {
                min_depth = std::min(min_depth, submission.sort_depth);
                max_depth = std::max(max_depth, submission.sort_depth);
            }

            bin_submissions_by_depth(submissions, min_depth, max_depth, depth_bin_count);
        }
    }

    device.bind_lighting_uniforms(lighting_uniforms);

    for(const DrawSubmission& submission: submissions)
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
            0.f,
            0.f,
            0.f,
          },
        });
        device.draw_mesh(submission.mesh_handle);
    }
    device.clear_shadow_map();
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
    device.bind_camera_uniforms({
      .proj = projection,
      .view = view,
    });
    device.bind_lighting_uniforms({});
    device.bind_material_uniforms({
      .base_color = overlay_grid.color,
    });
    device.bind_shadow_uniforms({});

    device.draw_mesh(overlay_grid.mesh_handle);
}

bool Renderer::render_spotlight_depth_debug()
{
    if(shadow_map_texture == 0)
    {
        return false;
    }

    device.bind_rasterizer_state({
      .wireframe = false,
      .cull_face = false,
    });

    device.bind_shadow_map({
      .enabled = true,
      .texture_handle = shadow_map_texture,
      .clip_from_mesh = ml::mat4x4::identity(),
      .depth_bias = 0.f,
    });
    device.bind_material(overlay_spotlight_depth.material_handle);
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
      .params = {0.f, 0.f, 0.f, 0.f},
    });
    device.draw_mesh(overlay_spotlight_depth.mesh_handle);
    device.clear_shadow_map();

    return true;
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

    const bool has_shadow_camera = collect_shadow_camera(scene).has_value();
    if(has_shadow_camera)
    {
        render_shadow_map(scene);
    }

    if(display_settings.debug_spotlight_depth
       && has_shadow_camera
       && render_spotlight_depth_debug())
    {
        // Debug depth rendered.
    }
    else
    {
        render_scene(
          scene,
          camera,
          display_settings);
    }

    /*
     * viewport overlays.
     */

    if(!display_settings.debug_spotlight_depth
       && overlay_settings.show_grid)
    {
        render_grid(
          camera);
    }

    device.end_frame();

    render_time = std::chrono::duration<float>(
                    std::chrono::steady_clock::now() - render_start_time)
                    .count();
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
    comparative_state.saved_sort_mode = sort_mode;
    comparative_state.iterations_per_mode = iterations;
    comparative_state.active = true;

    // start first mode
    sort_mode = comparative_state.modes[comparative_state.current_mode_index];
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
                sort_mode = comparative_state.modes[comparative_state.current_mode_index];
                start_sorting_benchmark(scene, viewport, comparative_state.iterations_per_mode);
                return;
            }
            // finished comparative run
            comparative_state.active = false;
            sort_mode = comparative_state.saved_sort_mode;
        }

        return;
    }

    const auto camera_position =
      benchmark_state.camera_positions[benchmark_state.current_iteration
                                       % benchmark_state.camera_positions.size()];

    Camera& benchmark_camera = viewport.get_camera(scene);
    benchmark_camera.set_transform(
      make_camera_view_matrix(camera_position, target, up));
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
                sort_mode = comparative_state.modes[comparative_state.current_mode_index];
                start_sorting_benchmark(scene, viewport, comparative_state.iterations_per_mode);
            }
            else
            {
                comparative_state.active = false;
                sort_mode = comparative_state.saved_sort_mode;
            }
        }
    }
}

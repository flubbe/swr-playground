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
#include <limits>
#include <utility>
#include <vector>

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
    ml::mat4x4 view_from_mesh;
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
    auto light_pos_view = view * scene.get_light().position;

    std::vector<DrawSubmission> submissions;

    scene.for_each_object<StaticMesh>(
      [this, &display_settings, &projection, &view, &submissions](
        const StaticMesh& static_mesh)
      {
          ++render_stats.static_meshes;

          if(!static_mesh.has_mesh_sections())
          {
              return;
          }

          const auto obj_view = view * static_mesh.get_transform();
          const auto obj_clip = projection * obj_view;
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
                .view_from_mesh = obj_view,
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

    for(const DrawSubmission& submission: submissions)
    {
        device.bind_material(submission.material_handle);
        device.bind_uniforms({.proj = projection,
                              .view = submission.view_from_mesh,
                              .light_pos = light_pos_view});
        device.draw_mesh(submission.mesh_handle);
    }
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
      .light_pos = {},
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

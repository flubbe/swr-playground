/**
 * Software Rasterizer Playground.
 *
 * Main application.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <future>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "assets/shaders/color_flat.h"
#include "assets/shaders/color_smooth.h"
#include "assets/shaders/lit_smooth.h"
#include "assets/shaders/phong_smooth.h"
#include "assets/static_mesh_importer.h"
#include "containers/format.h"
#include "meshes/lod.h"
#include "renderer/material_manager.h"
#include "renderer/render_device.h"
#include "renderer/renderer.h"
#include "scene/gear.h"
#include "scene/scene.h"
#include "scene/static_mesh.h"
#include "serialization/file.h"
#include "tasks/task_system.h"
#include "ui/imgui.h"
#include "application.h"
#include "file_manager.h"
#include "logging.h"
#include "shader_factory.h"
#include "startup_tasks.h"
#include "staged_data.h"
#include "viewport.h"

using task_system::TaskCancelledError;
using task_system::TaskExecutionContext;
using task_system::TaskGroupSnapshot;
using task_system::TaskHandle;
using task_system::TaskSnapshot;
using task_system::TaskSpec;
using task_system::TaskState;

struct StartupMaterials
{
    ResolvableMaterial gear;
    ResolvableMaterial floor;
    ResolvableMaterial static_mesh;

    [[nodiscard]]
    bool is_ready()
    {
        using namespace std::chrono_literals;

        return gear.get_entry().wait_for(0ms) == std::future_status::ready
               && floor.get_entry().wait_for(0ms) == std::future_status::ready
               && static_mesh.get_entry().wait_for(0ms) == std::future_status::ready;
    }

    void wait()
    {
        gear.get_entry().wait();
        floor.get_entry().wait();
        static_mesh.get_entry().wait();
    }
};

namespace
{

struct DisplayProgress
{
    swr::string status_text;
    float progress{0.f};
};

[[nodiscard]]
swr::string task_display_text(const TaskSnapshot& task)
{
    if(!task.status_text.empty())
    {
        return task.status_text;
    }

    if(!task.name.empty())
    {
        return task.name;
    }

    switch(task.state)
    {
    case TaskState::Queued:
        return "Queued...";
    case TaskState::Running:
        return "Working...";
    case TaskState::Skipped:
        return "Skipped.";
    case TaskState::Completed:
        return "Done.";
    case TaskState::Cancelled:
        return "Cancelled.";
    case TaskState::Failed:
        return "Failed.";
    }

    return "Working...";
}

[[nodiscard]]
DisplayProgress summarize_task_display(
  const swr::vector<TaskSnapshot>& tasks,
  float progress,
  std::string_view default_status)
{
    std::size_t running_count = 0;
    const TaskSnapshot* running_task = nullptr;
    const TaskSnapshot* failed_task = nullptr;
    const TaskSnapshot* cancelled_task = nullptr;

    for(const TaskSnapshot& task: tasks)
    {
        if(task.state == TaskState::Failed && failed_task == nullptr)
        {
            failed_task = &task;
        }
        if(task.state == TaskState::Cancelled && cancelled_task == nullptr)
        {
            cancelled_task = &task;
        }
        if(task.state == TaskState::Running)
        {
            ++running_count;
            if(running_task == nullptr)
            {
                running_task = &task;
            }
        }
    }

    if(failed_task != nullptr)
    {
        return DisplayProgress{
          .status_text = task_display_text(*failed_task),
          .progress = progress,
        };
    }

    if(running_count == 1 && running_task != nullptr)
    {
        return DisplayProgress{
          .status_text = task_display_text(*running_task),
          .progress = progress,
        };
    }

    if(running_count > 1)
    {
        swr::string status_text = swr::format(
          "Waiting for {} running tasks...",
          running_count);

        return DisplayProgress{
          .status_text = std::move(status_text),
          .progress = progress,
        };
    }

    if(cancelled_task != nullptr)
    {
        return DisplayProgress{
          .status_text = task_display_text(*cancelled_task),
          .progress = progress,
        };
    }

    if(progress >= 1.f)
    {
        return DisplayProgress{
          .status_text = "Done.",
          .progress = progress,
        };
    }

    return DisplayProgress{
      .status_text = swr::string{default_status},
      .progress = progress,
    };
}

class SDLError final
: public std::runtime_error
{
public:
    explicit SDLError(
      std::string_view message)
    : std::runtime_error{
        std::format(
          "{}: {}",
          message,
          SDL_GetError())}
    {
    }
};

ViewportEditorCameraInput gather_viewport_camera_input(
  const ViewportInputState& viewport_input,
  const ImGuiIO& io,
  bool mouse_captured,
  ViewportNavigationMode mode)
{
    ViewportEditorCameraInput input{};

    if(!mouse_captured)
    {
        return input;
    }

    input.active = true;
    input.look_yaw = viewport_input.mouse_delta_x;
    input.look_pitch = viewport_input.mouse_delta_y;
    if(viewport_input.viewport_hovered)
    {
        input.zoom_delta = viewport_input.mouse_wheel_delta;
    }

    const bool keyboard_blocked = io.WantCaptureKeyboard;
    if(keyboard_blocked)
    {
        return input;
    }

    if(mode == ViewportNavigationMode::Orbit)
    {
        return input;
    }

    const bool* keys = SDL_GetKeyboardState(nullptr);
    if(keys == nullptr)
    {
        return input;
    }

    if(keys[SDL_SCANCODE_W])
    {
        input.move_forward += 1.f;
    }
    if(keys[SDL_SCANCODE_S])
    {
        input.move_forward -= 1.f;
    }
    if(keys[SDL_SCANCODE_D])
    {
        input.move_right += 1.f;
    }
    if(keys[SDL_SCANCODE_A])
    {
        input.move_right -= 1.f;
    }
    if(keys[SDL_SCANCODE_E])
    {
        input.move_up += 1.f;
    }
    if(keys[SDL_SCANCODE_Q])
    {
        input.move_up -= 1.f;
    }

    input.fast_move = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    return input;
}

GLuint create_viewport_texture(
  int width,
  int height)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    if(texture == 0)
    {
        throw std::runtime_error{"glGenTextures failed"};
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_SRGB8_ALPHA8,
      width,
      height,
      0,
      GL_BGRA,
      GL_UNSIGNED_INT_8_8_8_8_REV,
      nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void destroy_viewport_texture(GLuint& texture)
{
    if(texture != 0)
    {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}

void update_viewport_texture(
  GLuint texture,
  const RenderDevice& render_device)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
      GL_TEXTURE_2D,
      0,
      0,
      0,
      render_device.get_width(),
      render_device.get_height(),
      GL_BGRA,
      GL_UNSIGNED_INT_8_8_8_8_REV,
      render_device.get_data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void set_imgui_mouse_interactions_enabled(bool enabled)
{
    ImGuiIO& io = ImGui::GetIO();
    if(enabled)
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }
    else
    {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
}

bool viewport_contains_mouse_position(
  const ViewportInputState& viewport_input,
  float x,
  float y)
{
    return viewport_input.viewport_rect_valid
           && x >= viewport_input.viewport_min_x
           && x < viewport_input.viewport_max_x
           && y >= viewport_input.viewport_min_y
           && y < viewport_input.viewport_max_y;
}

void configure_default_directional_lights(Scene& scene);
void configure_default_spot_lights(Scene& scene);

void imgui_draw_viewport_panel(
  RenderDevice& render_device,
  Renderer& renderer,
  Scene& scene,
  Viewport& viewport,
  GLuint& viewport_texture,
  bool& running,
  ViewportInputState& viewport_input)
{
    ImGui::Begin("Viewport");

    ImVec2 avail = ImGui::GetContentRegionAvail();

    // ImGui sizes are logical units; rasterizer target should use pixels.
    int viewport_w_px = std::max(1, static_cast<int>(std::round(avail.x)));
    int viewport_h_px = std::max(1, static_cast<int>(std::round(avail.y)));

    // FIXME the dimensions should not come from the render device
    if(viewport_w_px != render_device.get_width()
       || viewport_h_px != render_device.get_height())
    {
        viewport.set_resolution(viewport_w_px, viewport_h_px);
        render_device.resize(viewport_w_px, viewport_h_px);

        destroy_viewport_texture(viewport_texture);

        try
        {
            viewport_texture = create_viewport_texture(
              render_device.get_width(),
              render_device.get_height());
            logging::logf(
              "resized viewport to {}x{}",
              render_device.get_width(),
              render_device.get_height());
        }
        catch(const std::exception& e)
        {
            logging::errorf("{}", e.what());
            running = false;
        }
    }

    viewport.update_active_camera_projection(scene);

    if(renderer.is_benchmark_in_progress())
    {
        renderer.update_sorting_benchmark(scene, viewport);
    }
    else
    {
        renderer.render(
          scene,
          viewport);
    }

    if(viewport_texture != 0)
    {
        update_viewport_texture(viewport_texture, render_device);

        // Display at logical UI size, not pixel size.
        ImGui::Image(
          static_cast<ImTextureID>(viewport_texture),
          avail,
          ImVec2{0, 0},
          ImVec2{1, 1});
        const ImVec2 viewport_min = ImGui::GetItemRectMin();
        const ImVec2 viewport_max = ImGui::GetItemRectMax();
        viewport_input.viewport_hovered = ImGui::IsItemHovered();
        viewport_input.viewport_rect_valid = true;
        viewport_input.viewport_min_x = viewport_min.x;
        viewport_input.viewport_min_y = viewport_min.y;
        viewport_input.viewport_max_x = viewport_max.x;
        viewport_input.viewport_max_y = viewport_max.y;

        if(renderer.is_benchmark_in_progress())
        {
            const std::size_t iteration =
              renderer.get_benchmark_current_iteration();
            const std::size_t target =
              renderer.get_benchmark_target_iterations();
            const char* phase = renderer.is_benchmark_sorted_phase()
                                  ? "With Sorting"
                                  : "Without Sorting";
            const swr::string status = swr::format(
              "Benchmark running: {} {}/{}",
              phase,
              iteration + 1,
              target);
            const ImVec2 text_pos = ImVec2{
              viewport_min.x + 8.0f,
              viewport_min.y + 28.0f};
            const ImVec2 text_size = ImGui::CalcTextSize(status.c_str());
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(
              ImVec2{text_pos.x - 4.f, text_pos.y - 4.f},
              ImVec2{text_pos.x + text_size.x + 4.f,
                     text_pos.y + text_size.y + 4.f},
              IM_COL32(0, 0, 0, 180),
              4.0f);
            draw_list->AddText(
              text_pos,
              IM_COL32(255, 255, 255, 255),
              status.c_str());
        }

        if(viewport.is_camera_selector_overlay_enabled())
        {
            const ViewportDisplaySettings display_settings =
              viewport.get_display_settings();
            const ViewportCameraType camera_type = viewport.get_camera_type(scene);
            swr::string camera_name{to_string(viewport.get_editor_camera_view())};
            if(display_settings.debug_spotlight_depth)
            {
                camera_name = "Spotlight Depth";
            }
            else if(camera_type == ViewportCameraType::Scene)
            {
                camera_name = viewport.get_camera(scene).get_name();
            }

            const swr::string label_left = "[";
            const swr::string label_name = camera_name;
            const swr::string label_right = "]";
            const swr::string label = swr::format(
              "[{}]",
              label_name);
            const ImVec2 text_pos = ImVec2{
              viewport_min.x + 8.0f,
              viewport_min.y + 6.0f};

            // Context-menu trigger area over camera label (right-click like DCC/CAD tools).
            const ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
            ImGui::SetCursorScreenPos(text_pos);
            ImGui::InvisibleButton("viewport_camera_overlay_menu_trigger", label_size);
            const bool is_hovered = ImGui::IsItemHovered();
            const bool is_menu_open = ImGui::IsPopupOpen("viewport_camera_overlay_menu");
            const bool is_active = is_hovered || is_menu_open;
            if(is_hovered)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const ImU32 bracket_color = IM_COL32(235, 235, 235, 255);
            const ImU32 name_color = is_active
                                       ? IM_COL32(255, 224, 120, 255)
                                       : bracket_color;
            const ImVec2 left_size = ImGui::CalcTextSize(label_left.c_str());
            const ImVec2 name_size = ImGui::CalcTextSize(label_name.c_str());

            draw_list->AddText(
              text_pos,
              bracket_color,
              label_left.c_str());
            draw_list->AddText(
              ImVec2{text_pos.x + left_size.x, text_pos.y},
              name_color,
              label_name.c_str());
            draw_list->AddText(
              ImVec2{text_pos.x + left_size.x + name_size.x, text_pos.y},
              bracket_color,
              label_right.c_str());
            if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                ImGui::OpenPopup("viewport_camera_overlay_menu");
            }
            if(ImGui::BeginPopup("viewport_camera_overlay_menu"))
            {
                ViewportDisplaySettings display_settings = viewport.get_display_settings();
                const bool showing_spotlight_depth =
                  display_settings.debug_spotlight_depth;
                bool update_display_settings = false;

                for(int view_index = 0;
                    view_index <= static_cast<int>(EditorCameraView::Orthographic);
                    ++view_index)
                {
                    const auto view = static_cast<EditorCameraView>(view_index);
                    if(ImGui::MenuItem(
                         to_string(view).data(),
                         nullptr,
                         !showing_spotlight_depth
                           && viewport.is_editor_camera_view_active(scene, view)))
                    {
                        display_settings.debug_spotlight_depth = false;
                        update_display_settings = true;
                        viewport.use_local_camera();
                        viewport.set_editor_camera_view(view);
                    }
                }

                ImGui::Separator();
                if(ImGui::BeginMenu("Scene Cameras"))
                {
                    bool has_any_scene_camera = false;

                    for(const auto& camera: scene.objects_of<Camera>())
                    {
                        has_any_scene_camera = true;
                        if(ImGui::MenuItem(
                             camera.get_name().c_str(),
                             nullptr,
                             !showing_spotlight_depth
                               && viewport.is_scene_camera_active(
                                 scene,
                                 camera.get_object_id())))
                        {
                            display_settings.debug_spotlight_depth = false;
                            update_display_settings = true;
                            viewport.use_scene_camera(camera.get_object_id());
                        }
                    }

                    if(!has_any_scene_camera)
                    {
                        ImGui::BeginDisabled(true);
                        ImGui::MenuItem("<No Scene Cameras>");
                        ImGui::EndDisabled();
                    }

                    ImGui::EndMenu();
                }

                const bool using_scene_camera =
                  camera_type == ViewportCameraType::Scene;
                ImGui::Separator();
                if(using_scene_camera)
                {
                    ImGui::BeginDisabled();
                }
                if(ImGui::MenuItem("Reset Cameras") && !using_scene_camera)
                {
                    display_settings.debug_spotlight_depth = false;
                    update_display_settings = true;
                    viewport.reset_editor_camera();
                }
                if(using_scene_camera)
                {
                    ImGui::EndDisabled();
                }

                if(ImGui::BeginMenu("Debug"))
                {
                    if(ImGui::MenuItem(
                         "Spotlight Depth",
                         nullptr,
                         display_settings.debug_spotlight_depth))
                    {
                        display_settings.debug_spotlight_depth =
                          !display_settings.debug_spotlight_depth;
                        update_display_settings = true;
                    }
                    ImGui::EndMenu();
                }

                if(update_display_settings)
                {
                    viewport.set_display_settings(display_settings);
                }

                ImGui::EndPopup();
            }
        }
    }
    else
    {
        viewport_input.viewport_hovered = false;
        viewport_input.viewport_rect_valid = false;
    }

    ImGui::End();
}

/*
 * Startup scene finalization.
 */

GearParameters create_gear_resources(
  RenderDevice& device,
  ResolvableMaterial material,
  const StagedGearInstance& staged)
{
    auto inner_mesh_data = MeshData{
      .primitive_type = PrimitiveType::Triangles,
      .indices = staged.geometry.inner_indices,
      .vertices = staged.geometry.inner_vertices,
      .normals = staged.geometry.inner_normals,
      .texcoords = {}};
    auto inner_mesh = device.create_mesh(inner_mesh_data);

    auto outer_mesh_data = MeshData{
      .primitive_type = PrimitiveType::Triangles,
      .indices = staged.geometry.outer_indices,
      .vertices = staged.geometry.outer_vertices,
      .normals = staged.geometry.outer_normals,
      .texcoords = {}};
    auto outer_mesh = device.create_mesh(outer_mesh_data);

    MeshBounds bounds = calculate_mesh_bounds(inner_mesh_data);
    expand_bounds(bounds, calculate_mesh_bounds(outer_mesh_data));

    return GearParameters{
      .inner = MeshSection{
        .mesh_handle = inner_mesh,
        .material = material,
        .color = staged.color,
        .triangle_count = inner_mesh_data.indices.size() / 3,
      },
      .outer = MeshSection{
        .mesh_handle = outer_mesh,
        .material = material,
        .color = staged.color,
        .triangle_count = outer_mesh_data.indices.size() / 3,
      },
      .bounds = bounds,
      .inner_radius = staged.inner_radius,
      .outer_radius = staged.outer_radius,
      .width = staged.width,
      .teeth = staged.teeth,
      .tooth_depth = staged.tooth_depth,
    };
}

void add_staged_gears(
  Scene& scene,
  RenderDevice& device,
  ResolvableMaterial material,
  const swr::vector<StagedGearInstance>& gears)
{
    for(const StagedGearInstance& staged: gears)
    {
        auto params = create_gear_resources(
          device,
          material,
          staged);
        auto* gear = scene.add_object<Gear>(params);
        gear->casts_shadows = true;
        gear->set_transform(staged.transform);
        scene.set_spin_animation(
          gear->get_object_id(),
          {.translation = staged.translation,
           .angular_speed = staged.angular_speed,
           .phase_offset = staged.phase_offset});
    }
}

constexpr std::string_view floor_object_name = "Stone Floor";

swr::vector<StaticMeshLod> create_static_mesh_resources(
  RenderDevice& device,
  ResolvableMaterial material,
  const StagedStaticMeshAsset& staged_asset)
{
    swr::vector<StaticMeshLod> result_lods;
    if(staged_asset.sections.empty())
    {
        return result_lods;
    }

    result_lods.resize(staged_asset.sections.front().lods.size());

    for(std::size_t i = 0; i < result_lods.size(); ++i)
    {
        result_lods[i].triangle_count =
          staged_asset.sections.front().lods[i].mesh.indices.size() / 3;
    }

    for(const StagedStaticMeshSection& section: staged_asset.sections)
    {
        for(std::size_t lod_index = 0;
            lod_index < section.lods.size() && lod_index < result_lods.size();
            ++lod_index)
        {
            const StagedStaticMeshSectionLod& staged_lod =
              section.lods[lod_index];
            const MeshHandle mesh_handle = device.create_mesh(staged_lod.mesh);
            expand_bounds(
              result_lods[lod_index].bounds,
              staged_lod.bounds);
            result_lods[lod_index].mesh_sections.push_back(
              MeshSection{
                .mesh_handle = mesh_handle,
                .material = material,
                .color = section.diffuse_color,
                .triangle_count = staged_lod.mesh.indices.size() / 3,
              });
        }
    }

    std::erase_if(
      result_lods,
      [](const StaticMeshLod& lod)
      {
          return lod.mesh_sections.empty();
      });

    return result_lods;
}

void try_add_textured_floor(
  Scene& scene,
  RenderDevice& device,
  ResolvableMaterial material,
  const StagedFloorData& floor_data)
{
    std::optional<MeshHandle> mesh_handle;

    try
    {
        constexpr float floor_height = -6.25f;

        mesh_handle = device.create_mesh(
          floor_data.mesh);

        auto* floor = scene.add_object<StaticMesh>(
          "",
          swr::vector<MeshSection>{
            MeshSection{
              .mesh_handle = *mesh_handle,
              .material = material,
              .color = {1.f, 1.f, 1.f, 1.f},
              .triangle_count = floor_data.mesh.indices.size() / 3,
            }},
          calculate_mesh_bounds(floor_data.mesh));
        floor->set_name(floor_object_name);
        floor->casts_shadows = false;
        floor->set_transform(ml::matrices::translation(0.f, floor_height, 0.f));
        floor->capture_snapshot();
    }
    catch(const std::exception& e)
    {
        if(mesh_handle.has_value())
        {
            device.delete_mesh(*mesh_handle);
        }
        logging::warningf(
          "failed to create textured floor: {}",
          e.what());
    }
}

StaticMesh* create_static_mesh_instance(
  std::string_view path,
  Scene& scene,
  const StagedStaticMeshAsset& resources,
  swr::vector<StaticMeshLod> lods,
  const ml::mat4x4& transform)
{
    StaticMesh* mesh = scene.add_object<StaticMesh>(
      path,
      std::move(lods));
    mesh->set_name(resources.name);
    mesh->set_transform(transform * resources.fit_transform);
    mesh->capture_snapshot();
    return mesh;
}

void finalize_startup_scene(
  Scene& scene,
  Viewport& viewport,
  RenderDevice& render_device,
  StartupMaterials& startup_materials,
  const StagedStartupScene& staged_scene)
{
    configure_default_directional_lights(scene);
    configure_default_spot_lights(scene);

    add_staged_gears(
      scene,
      render_device,
      startup_materials.gear,
      staged_scene.gears);

    if(staged_scene.floor.has_value())
    {
        try_add_textured_floor(
          scene,
          render_device,
          startup_materials.floor,
          *staged_scene.floor);
    }

    if(staged_scene.sample_mesh.has_value())
    {
        auto lods = create_static_mesh_resources(
          render_device,
          startup_materials.static_mesh,
          *staged_scene.sample_mesh);

        if(!lods.empty())
        {
            StaticMesh* sample_mesh = create_static_mesh_instance(
              staged_scene.sample_mesh.value().path,
              scene,
              *staged_scene.sample_mesh,
              std::move(lods),
              ml::matrices::translation(0.f, 0.f, 5.f));
            sample_mesh->casts_shadows = true;
        }
    }

    Camera* camera = scene.add_object<Camera>();
    camera->set_transform(viewport.get_local_camera().get_transform());
    camera->set_name("Editor Camera");
    camera->capture_snapshot();

    viewport.use_local_camera();
}

template<
  typename Rep,
  typename Period>
TaskSpec make_wait_task(
  swr::string name,
  int iterations,
  std::chrono::duration<Rep, Period> per_iteration,
  float weight)
{
    const auto task_name = name;

    return TaskSpec{
      .name = task_name,
      .weight = weight,
      .run = [task_name, iterations, per_iteration](TaskExecutionContext& context)
      {
          const int safe_iterations = std::max(1, iterations);
          for(int i = 0; i < safe_iterations; ++i)
          {
              if(context.is_cancel_requested())
              {
                  throw TaskCancelledError{};
              }

              std::this_thread::sleep_for(per_iteration);
              context.update(
                std::format(
                  "{} ({}/{})",
                  task_name,
                  i + 1,
                  safe_iterations),
                static_cast<float>(i + 1) / static_cast<float>(safe_iterations));
          }
      },
    };
}

void rebuild_gear_mesh_if_needed(
  RenderDevice& device,
  Gear* gear)
{
    if(gear == nullptr)
    {
        return;
    }

    gear->clamp_runtime_parameters();
    if(!gear->needs_rebuild())
    {
        return;
    }

    const int teeth = std::clamp(
      gear->get_teeth(),
      gear_limits::min_teeth,
      gear_limits::max_teeth);

    // FIXME The copy is here only to update the triangle count below.
    auto mesh_lods = gear->get_lods();

    // validate assumptions since we'd like to reuse the mesh handle.
    if(mesh_lods.size() != 1
       || mesh_lods[0].mesh_sections.size() != 2)
    {
        logging::errorf("Cannot rebuild gear mesh: LOD parameters do not match.");
        return;
    }

    auto geom = make_gear(
      gear->get_inner_radius(),
      gear->get_outer_radius(),
      gear->get_width(),
      teeth,
      gear->get_tooth_depth());

    const MeshHandle old_inner_mesh = mesh_lods[0].mesh_sections[0].mesh_handle;
    const MeshHandle old_outer_mesh = mesh_lods[0].mesh_sections[1].mesh_handle;

    const bool inner_updated = device.update_mesh(
      old_inner_mesh,
      MeshData{
        .primitive_type = PrimitiveType::Triangles,
        .indices = geom.inner_indices,
        .vertices = geom.inner_vertices,
        .normals = geom.inner_normals,
        .texcoords = {}});

    std::uint32_t inner_triangle_count = geom.inner_indices.size() / 3;
    if(!inner_updated)
    {
        // mesh was not updated.
        inner_triangle_count = mesh_lods[0].mesh_sections[0].triangle_count;
    }

    const bool outer_updated = device.update_mesh(
      old_outer_mesh,
      MeshData{
        .primitive_type = PrimitiveType::Triangles,
        .indices = geom.outer_indices,
        .vertices = geom.outer_vertices,
        .normals = geom.outer_normals,
        .texcoords = {}});

    std::uint32_t outer_triangle_count = geom.outer_indices.size() / 3;
    if(!outer_updated)
    {
        // mesh was not updated.
        outer_triangle_count = mesh_lods[0].mesh_sections[1].triangle_count;
    }

    // update triangle count.
    mesh_lods[0].mesh_sections[0].triangle_count = inner_triangle_count;
    mesh_lods[0].mesh_sections[1].triangle_count = outer_triangle_count;
    gear->set_lods(mesh_lods);

    gear->mark_rebuilt();
}

void configure_default_directional_lights(Scene& scene)
{
    auto* key_light = scene.add_object<DirectionalLight>();
    key_light->set_name("Key Light");
    key_light->behavior = DirectionalLightBehavior::Rotating;
    key_light->brightness = 0.55f;
    key_light->set_transform(
      ml::matrices::rotation_y(ml::to_radians(210.f))
      * ml::matrices::rotation_x(ml::to_radians(-35.f)));
    key_light->set_position({5.f, 8.f, 10.f});
    key_light->capture_snapshot();

    auto* fill_light = scene.add_object<DirectionalLight>();
    fill_light->set_name("Fill Light");
    fill_light->behavior = DirectionalLightBehavior::Stationary;
    fill_light->brightness = 0.6f;
    fill_light->set_transform(
      ml::matrices::rotation_y(ml::to_radians(35.f))
      * ml::matrices::rotation_x(ml::to_radians(-55.f)));
    fill_light->set_position({-10.f, 12.f, -6.f});
    fill_light->capture_snapshot();
}

void configure_default_spot_lights(Scene& scene)
{
    auto* spotlight = scene.add_object<SpotLight>();
    spotlight->set_name("Spot Light");
    spotlight->casts_shadows = true;
    spotlight->color = {1.f, 1.f, 1.f, 1.f};
    spotlight->brightness = 2.4f;
    spotlight->inner_cone_angle_radians = ml::to_radians(20.f);
    spotlight->outer_cone_angle_radians = ml::to_radians(21.f);
    spotlight->range = 45.f;

    const ml::vec3 spotlight_position{0.f, 11.f, 12.f};
    const ml::vec3 direction_to_origin =
      (-spotlight_position).normalized();
    const float spotlight_pitch =
      std::asin(direction_to_origin.y);
    const float spotlight_yaw =
      std::atan2(-direction_to_origin.x, -direction_to_origin.z);

    spotlight->set_transform(
      ml::matrices::rotation_y(spotlight_yaw)
      * ml::matrices::rotation_x(spotlight_pitch));
    spotlight->set_position(spotlight_position);
    spotlight->capture_snapshot();
}

DisplayProgress aggregate_startup_progress(
  const swr::vector<TaskHandle>& handles,
  const swr::vector<float>& weights)
{
    if(handles.empty() || handles.size() != weights.size())
    {
        return DisplayProgress{
          .status_text = "Starting...",
          .progress = 0.f,
        };
    }

    float total_weight = 0.f;
    float completed_weight = 0.f;
    swr::vector<TaskSnapshot> task_snapshots;

    for(std::size_t i = 0; i < handles.size(); ++i)
    {
        const float weight = std::max(1.f, weights[i]);
        const TaskGroupSnapshot task_group_snapshot = handles[i].snapshot();
        const float task_progress = std::clamp(task_group_snapshot.progress, 0.f, 1.f);

        total_weight += weight;
        completed_weight += task_progress * weight;

        task_snapshots.insert(
          task_snapshots.end(),
          task_group_snapshot.tasks.begin(),
          task_group_snapshot.tasks.end());
    }

    if(total_weight <= 0.f)
    {
        return DisplayProgress{
          .status_text = "Starting...",
          .progress = 0.f,
        };
    }

    return summarize_task_display(
      task_snapshots,
      std::clamp(completed_weight / total_weight, 0.f, 1.f),
      "Loading scene");
}

}    // namespace

/*
 * ApplicationTaskSystemLogger.
 */

ApplicationTaskSystemLogger::ApplicationTaskSystemLogger(
  logging::LogDevice& log_device)
: logger{"TaskSystem", log_device}
{
}

void ApplicationTaskSystemLogger::log(std::string_view message) const
{
    logger.logf("{}", message);
}

void ApplicationTaskSystemLogger::warn(std::string_view message) const
{
    logger.warningf("{}", message);
}

void ApplicationTaskSystemLogger::error(std::string_view message) const
{
    logger.errorf("{}", message);
}

/*
 * Application.
 */

void Application::show_window()
{
    if(window == nullptr)
    {
        logging::errorf("Cannot show window: No window.");
        return;
    }

    if(!SDL_ShowWindow(window))
    {
        logging::errorf(
          "SDL_ShowWindow failed: {}",
          SDL_GetError());
    }
}

void Application::hide_window()
{
    if(window == nullptr)
    {
        logging::errorf("Cannot hide window: No window.");
        return;
    }

    if(!SDL_HideWindow(window))
    {
        logging::errorf(
          "SDL_HideWindow failed: {}",
          SDL_GetError());
    }
}

bool Application::is_window_shown() const
{
    if(window == nullptr)
    {
        logging::errorf("Cannot query window flags: No window.");
        return false;
    }

    return (SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0;
}

swr::string Application::get_startup_status() const
{
    return aggregate_startup_progress(
             startup_task_handles,
             startup_task_weights)
      .status_text;
}

bool Application::pump_messages()
{
    bool running = true;

    viewport_input.mouse_delta_x = 0.f;
    viewport_input.mouse_delta_y = 0.f;
    viewport_input.mouse_wheel_delta = 0.f;

    SDL_Event event;
    bool suppress_imgui_mouse = viewport_mouse_captured;
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
           && event.button.button == SDL_BUTTON_RIGHT
           && viewport.is_editor_camera_modification_enabled()
           && viewport.is_local_camera_active(scene)
           && viewport_contains_mouse_position(
             viewport_input,
             event.button.x,
             event.button.y))
        {
            set_viewport_mouse_capture(true);
        }
        suppress_imgui_mouse = suppress_imgui_mouse || viewport_mouse_captured;

        ImGui_ImplSDL3_ProcessEvent(&event);

        if(event.type == SDL_EVENT_QUIT)
        {
            set_viewport_mouse_capture(false);
            running = false;
        }
        else if(event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
        {
            set_viewport_mouse_capture(false);
        }
        else if(event.type == SDL_EVENT_MOUSE_BUTTON_UP
                && event.button.button == SDL_BUTTON_RIGHT)
        {
            set_viewport_mouse_capture(false);
        }
        else if(event.type == SDL_EVENT_MOUSE_MOTION)
        {
            viewport_input.mouse_delta_x += event.motion.xrel;
            viewport_input.mouse_delta_y += event.motion.yrel;
        }
        else if(event.type == SDL_EVENT_MOUSE_WHEEL)
        {
            viewport_input.mouse_wheel_delta += event.wheel.y;
        }
    }

    set_imgui_mouse_interactions_enabled(!suppress_imgui_mouse);
    return running;
}

void Application::prepare_frame()
{
    memory::frame_bump()->reset();
    memory::frame_arena()->reset();

    material_manager.process_pending();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void Application::render_frame()
{
    update_runtime_test_task();

    imgui::draw_main_dockspace(*this);
    imgui_draw_viewport_panel(
      render_device,
      renderer,
      scene,
      viewport,
      viewport_texture,
      viewport_panel_running,
      viewport_input);
    imgui::draw_console_panel(log_device);
    imgui::draw_tools_panel(
      *this,
      render_device,
      viewport,
      scene,
      renderer,
      frame_index,
      pixel_density);
    imgui::draw_profiler_panel(renderer);
    imgui::draw_memory_profiler_panel();

    if(imgui::check_and_clear_sorting_benchmark_request())
    {
        renderer.start_sorting_benchmark(
          scene,
          viewport,
          benchmark_iterations);
    }

    imgui::draw_scene_inspector_panel(ui_state, scene);
    imgui::draw_class_inspector_panel(ui_state);

    draw_runtime_test_modal();

    ImGui::Render();

    SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);
    glViewport(0, 0, pixel_w, pixel_h);
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);

    ++frame_index;
}

void Application::on_startup_complete(const StagedStartupScene& staged_scene)
{
    const logging::Logger startup_logger{"Startup"};

    for(const auto& notice: staged_scene.notices)
    {
        startup_logger.warningf("{}", notice);
    }
    finalize_startup_scene(
      scene,
      viewport,
      render_device,
      *startup_materials,
      staged_scene);
    scene.add_default_systems();
    setup_viewport();
}

void Application::on_startup_complete_error(const std::string& error_message)
{
    const logging::Logger startup_logger{"Startup"};
    startup_logger.errorf("Loading failed: {}", error_message);
    startup_error = error_message;
}

void Application::setup_viewport()
{
    // Keep the local camera initialized as a fallback if the bound scene camera is removed.
    viewport.reset_editor_camera();
}

Application::Application(
  std::string_view title,
  logging::BufferedLogDevice& log_device,
  FileManager& file_manager,
  task_system::TaskSystem& task_system,
  RenderDevice& render_device,
  Renderer& renderer,
  MaterialManager& material_manager,
  Scene& scene,
  Viewport& viewport)
: title{title}
, log_device{log_device}
, file_manager{file_manager}
, task_system{task_system}
, render_device{render_device}
, renderer{renderer}
, material_manager{material_manager}
, scene{scene}
, viewport{viewport}
{
    window = SDL_CreateWindow(
      this->title.c_str(),
      1280,    // TODO load from config
      800,     // TODO load from config
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
    if(!window)
    {
        throw SDLError{"SDL_CreateWindow failed"};
    }

    gl_context = SDL_GL_CreateContext(window);
    if(!gl_context)
    {
        throw SDLError{"SDL_GL_CreateContext failed"};
    }

    if(!SDL_GL_MakeCurrent(window, gl_context))
    {
        throw SDLError{"SDL_GL_MakeCurrent failed"};
    }

    SDL_GL_SetSwapInterval(1);

    if(!imgui::init(window, gl_context))
    {
        throw std::runtime_error{"imgui::init failed."};
    }

    pixel_density = SDL_GetWindowPixelDensity(window);
    display_scale = SDL_GetWindowDisplayScale(window);

    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);

    logging::logf("display scale: {}", display_scale);
    logging::logf("pixel density: {}", pixel_density);
    logging::logf("window size: {} x {}", window_w, window_h);
    logging::logf("pixel size: {} x {}", pixel_w, pixel_h);

    /*
     * viewport setup.
     */

    viewport_texture = create_viewport_texture(
      render_device.get_width(),
      render_device.get_height());
    viewport.set_resolution(
      render_device.get_width(),
      render_device.get_height());

    setup_viewport();
}

Application::~Application()
{
    if(runtime_test_task_handle.valid())
    {
        runtime_test_task_handle.cancel();
        runtime_test_task_handle.wait();
    }
    runtime_test_task_handle = TaskHandle{};
    runtime_test_task_future = std::future<void>{};

    cancel_startup();
    startup_error.reset();

    set_viewport_mouse_capture(false);

    imgui::shutdown();

    destroy_viewport_texture(viewport_texture);

    if(gl_context)
    {
        SDL_GL_DestroyContext(gl_context);
    }
    if(window)
    {
        SDL_DestroyWindow(window);
    }
}

void Application::set_viewport_mouse_capture(
  bool enabled)
{
    if(viewport_mouse_captured == enabled || window == nullptr)
    {
        return;
    }

    if(enabled)
    {
        SDL_GetMouseState(
          &viewport_mouse_restore_x,
          &viewport_mouse_restore_y);
        viewport_mouse_restore_valid = true;
    }
    else if(viewport_mouse_restore_valid)
    {
        // SDL recommends warping before disabling relative mode when restoring cursor position.
        SDL_WarpMouseInWindow(
          window,
          viewport_mouse_restore_x,
          viewport_mouse_restore_y);
    }

    if(!SDL_SetWindowRelativeMouseMode(window, enabled))
    {
        if(enabled)
        {
            viewport_mouse_restore_valid = false;
        }
        logging::warningf(
          "failed to {} viewport mouse capture: {}",
          enabled ? "enable" : "disable",
          SDL_GetError());
        return;
    }

    viewport_mouse_captured = enabled;
    if(!enabled)
    {
        viewport_mouse_restore_valid = false;
    }
    viewport_input.mouse_delta_x = 0.f;
    viewport_input.mouse_delta_y = 0.f;
}

void Application::update_viewport_mouse_capture()
{
    if(!viewport.is_editor_camera_modification_enabled()
       || !viewport.is_local_camera_active(scene))
    {
        set_viewport_mouse_capture(false);
        return;
    }

    float mouse_x = 0.f;
    float mouse_y = 0.f;
    const SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    const bool right_mouse_down = (mouse_buttons & SDL_BUTTON_RMASK) != 0;
    const bool should_capture =
      viewport_mouse_captured
        ? right_mouse_down
        : viewport_contains_mouse_position(viewport_input, mouse_x, mouse_y)
            && right_mouse_down;

    set_viewport_mouse_capture(should_capture);
}

void Application::begin_startup()
{
    /*
     * Startup / initialization.
     */

    cancel_startup();
    startup_error.reset();

    // Load materials.
    const std::string_view floor_material_path = "assets/materials/floor/floor.json";
    const std::string_view shadowed_material_path = "assets/materials/mesh/lit.json";

    auto floor_material = material_manager.load(
      floor_material_path,
      read_text_file(file_manager, floor_material_path));
    auto shadowed_material = material_manager.load(
      shadowed_material_path,
      read_text_file(file_manager, shadowed_material_path));

    startup_materials = swr::make_unique<StartupMaterials>(
      StartupMaterials{
        .gear = shadowed_material,
        .floor = floor_material,
        .static_mesh = shadowed_material,
      });

    startup_scene = std::make_shared<StagedStartupScene>();
    auto tasks = startup_tasks::create_startup_tasks(*startup_scene);

    startup_task_handles.clear();
    startup_task_futures.clear();
    startup_task_weights.clear();
    startup_task_handles.reserve(tasks.size());
    startup_task_futures.reserve(tasks.size());
    startup_task_weights.reserve(tasks.size());

    for(TaskSpec& task: tasks)
    {
        startup_task_weights.push_back(std::max(task.weight, 1.f));

        auto startup_submission = task_system.submit(
          [task = std::move(task)](TaskExecutionContext& context) mutable
          {
              if(context.is_cancel_requested())
              {
                  throw TaskCancelledError{};
              }

              if(!task.name.empty())
              {
                  context.update(task.name, 0.f);
              }

              if(task.run)
              {
                  task.run(context);
              }

              if(context.is_cancel_requested())
              {
                  throw TaskCancelledError{};
              }

              if(!task.name.empty())
              {
                  context.update(task.name, 1.f);
              }
          });

        startup_task_handles.push_back(startup_submission.handle);
        startup_task_futures.push_back(std::move(startup_submission.future));
    }
}

bool Application::is_startup_ready() const
{
    using namespace std::literals;

    if(startup_task_futures.empty() || startup_materials == nullptr)
    {
        return false;
    }

    for(const auto& startup_task_future: startup_task_futures)
    {
        if(!startup_task_future.valid()
           || startup_task_future.wait_for(0ms)
                != std::future_status::ready)
        {
            return false;
        }
    }

    return startup_materials->is_ready();
}

bool Application::finish_startup_if_ready()
{
    if(!is_startup_ready())
    {
        return false;
    }

    try
    {
        for(auto& startup_task_future: startup_task_futures)
        {
            if(startup_task_future.valid())
            {
                startup_task_future.get();
            }
        }

        if(startup_scene == nullptr)
        {
            throw std::runtime_error{"startup scene state is missing"};
        }
        if(startup_materials == nullptr)
        {
            throw std::runtime_error{"startup material state is missing"};
        }

        startup_materials->wait();
        on_startup_complete(*startup_scene);
        startup_task_handles.clear();
        startup_task_futures.clear();
        startup_task_weights.clear();
        startup_scene.reset();
        startup_materials.reset();
    }
    catch(const std::exception& e)
    {
        on_startup_complete_error(e.what());
        startup_task_handles.clear();
        startup_task_futures.clear();
        startup_task_weights.clear();
        startup_scene.reset();
        startup_materials.reset();

        throw;
    }

    return true;
}

void Application::cancel_startup()
{
    for(const TaskHandle& startup_task_handle: startup_task_handles)
    {
        startup_task_handle.cancel();
    }

    for(const TaskHandle& startup_task_handle: startup_task_handles)
    {
        startup_task_handle.wait();
    }

    startup_task_handles.clear();
    startup_task_futures.clear();
    startup_task_weights.clear();
    startup_scene.reset();
    startup_materials.reset();
}

void Application::start_debug_test_tasks()
{
    using namespace std::literals;

    if(runtime_test_task_handle.valid())
    {
        return;
    }

    runtime_test_task_error.reset();
    runtime_test_modal_open = true;

    swr::vector<TaskSpec> tasks;
    tasks.reserve(3);

    tasks.push_back(make_wait_task(
      "Loading assets...",
      8,
      100ms,
      2.f));

    tasks.push_back(make_wait_task(
      "Preparing scene data...",
      10,
      90ms,
      3.f));

    TaskSpec finalizing = make_wait_task(
      "Finalizing...",
      6,
      110ms,
      1.f);
    finalizing.dependencies = {0, 1};
    tasks.push_back(std::move(finalizing));

    auto submission = task_system.submit_task_specs(std::move(tasks));

    runtime_test_task_handle = submission.handle;
    runtime_test_task_future = std::move(submission.future);
}

bool Application::is_debug_test_tasks_running() const noexcept
{
    return runtime_test_task_handle.valid();
}

void Application::update_runtime_test_task()
{
    using namespace std::literals;

    if(!runtime_test_task_future.valid())
    {
        return;
    }

    if(runtime_test_task_future.wait_for(0ms)
       != std::future_status::ready)
    {
        return;
    }

    bool task_succeeded = true;
    try
    {
        runtime_test_task_future.get();
    }
    catch(const TaskCancelledError&)
    {
        task_succeeded = false;
        runtime_test_task_error = "Task run cancelled.";
    }
    catch(const std::exception& e)
    {
        task_succeeded = false;
        runtime_test_task_error = e.what();
    }

    if(task_succeeded)
    {
        runtime_test_modal_open = false;
        runtime_test_task_error.reset();
    }

    runtime_test_task_handle = TaskHandle{};
    runtime_test_task_future = std::future<void>{};
}

void Application::draw_runtime_test_modal()
{
    if(runtime_test_modal_open)
    {
        ImGui::OpenPopup("Loading...");
    }

    if(!ImGui::BeginPopupModal(
         "Loading...",
         nullptr,
         ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    if(!runtime_test_modal_open)
    {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    const TaskGroupSnapshot snapshot = runtime_test_task_handle.valid()
                                         ? runtime_test_task_handle.snapshot()
                                         : TaskGroupSnapshot{};
    const DisplayProgress progress = summarize_task_display(
      snapshot.tasks,
      snapshot.progress,
      "Working...");

    const char* status_text = progress.status_text.empty()
                                ? "Working..."
                                : progress.status_text.c_str();

    ImGui::TextUnformatted(status_text);
    ImGui::Spacing();
    ImGui::ProgressBar(progress.progress, ImVec2{320.f, 0.f});

    if(!snapshot.tasks.empty())
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for(const TaskSnapshot& task: snapshot.tasks)
        {
            const auto detail_text = task_display_text(task);
            swr::string label = task.name.empty()
                                  ? detail_text
                                  : swr::format("{}: {}", task.name, detail_text);

            ImGui::BulletText(
              "%s (%.0f%%)",
              label.c_str(),
              std::clamp(task.progress, 0.f, 1.f) * 100.f);
        }
    }

    if(runtime_test_task_handle.valid())
    {
        ImGui::Spacing();
        if(ImGui::Button("Cancel", ImVec2{120.f, 0.f}))
        {
            runtime_test_task_handle.cancel();
        }
    }
    else
    {
        if(runtime_test_task_error.has_value())
        {
            ImGui::Spacing();
            ImGui::TextWrapped("%s", runtime_test_task_error->c_str());
        }

        ImGui::Spacing();
        if(ImGui::Button("Close", ImVec2{120.f, 0.f}))
        {
            runtime_test_modal_open = false;
            runtime_test_task_error.reset();
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();
}

void Application::tick(float delta_time)
{
    update_viewport_mouse_capture();
    const ViewportNavigationMode navigation_mode = viewport.get_navigation_mode();
    const ViewportEditorCameraInput controller_input =
      viewport.is_editor_camera_modification_enabled()
        ? gather_viewport_camera_input(
            viewport_input,
            ImGui::GetIO(),
            viewport_mouse_captured,
            navigation_mode)
        : ViewportEditorCameraInput{};
    viewport.update_editor_camera(
      delta_time,
      controller_input);

    // Handle SPACE key for play/pause toggle
    const bool* keys = SDL_GetKeyboardState(nullptr);
    const bool space_pressed = keys != nullptr && keys[SDL_SCANCODE_SPACE];
    if(space_pressed && !prev_space_pressed)
    {
        scene.set_paused(!scene.is_paused());
    }
    prev_space_pressed = space_pressed;

    // FIXME temporary until a better update mechanism is in place
    scene.for_each_object<Gear>(
      [&](Gear& gear)
      {
          rebuild_gear_mesh_if_needed(render_device, &gear);
      });

    scene.tick(delta_time);
}

void Application::set_static_mesh_shader(StaticMeshShaderType type)
{
    active_static_mesh_shader = type;

    const swr::string material_path = [&]() -> swr::string
    {
        if(type == StaticMeshShaderType::ColorFlat)
        {
            return "assets/materials/mesh/flat.json";
        }
        else if(type == StaticMeshShaderType::ColorSmooth)
        {
            return "assets/materials/mesh/smooth.json";
        }
        else if(type == StaticMeshShaderType::PhongSmooth)
        {
            return "assets/materials/mesh/phong.json";
        }
        else if(type == StaticMeshShaderType::LitSmooth)
        {
            return "assets/materials/mesh/lit.json";
        }
        else
        {
            throw std::runtime_error{"Unknown mesh material."};
        }
    }();

    auto get_material = [&]() -> ResolvableMaterial
    {
        // Avoid filesystem access.
        auto cached_material = material_manager.get(material_path);
        if(cached_material.has_value())
        {
            return cached_material.value();
        }

        auto json = read_text_file(file_manager, material_path);
        return material_manager.load(material_path, json);
    };

    for(auto& mesh: scene.objects_of<StaticMesh>())
    {
        // skip gears for now.
        if(mesh.is_a<Gear>())
        {
            continue;
        }
        if(mesh.get_name() == floor_object_name)
        {
            continue;
        }

        for(auto& lod: mesh.get_lods())
        {
            for(auto& section: lod.mesh_sections)
            {
                section.material = get_material();
            }
        }
    }
}

void Application::set_floor_shader(FloorShaderType type)
{
    active_floor_shader = type;

    const swr::string path = [&]() -> swr::string
    {
        if(type == FloorShaderType::TexturedFloor)
        {
            return "assets/materials/floor/floor.json";
        }
        else if(type == FloorShaderType::TexturedShinyFloor)
        {
            return "assets/materials/floor/shiny_floor.json";
        }
        else
        {
            throw std::runtime_error{"Unknown floor shader type."};
        }
    }();

    const ResolvableMaterial material = [&]() -> ResolvableMaterial
    {
        // Avoid filesystem access.
        auto cached_material = material_manager.get(path);
        if(cached_material.has_value())
        {
            return cached_material.value();
        }

        return material_manager.load(
          path,
          read_text_file(file_manager, path));
    }();

    for(auto& mesh: scene.objects_of<StaticMesh>())
    {
        if(mesh.get_name() != floor_object_name)
        {
            continue;
        }

        for(auto& lod: mesh.get_lods())
        {
            for(auto& section: lod.mesh_sections)
            {
                section.material = material;
            }
        }
    }
}

bool Application::load_scene(
  const std::filesystem::path& path)
{
    auto contents = read_text_file(file_manager, path);

    try
    {
        scene.load(contents);
    }
    catch(const std::runtime_error& e)
    {
        logging::errorf(
          "Failed to load scene from '{}': {}",
          path.string(),
          e.what());
        return false;
    }

    return true;
}

bool Application::save_scene(
  const std::filesystem::path& path)
{
    auto abs_path = file_manager.resolve_write(path);

    std::error_code ec;
    std::filesystem::create_directories(abs_path.parent_path(), ec);

    if(ec)
    {
        logging::errorf(
          "Failed to create directory structure '{}': {}",
          abs_path.parent_path().string(),
          ec.message());

        return false;
    }

    write_text_file(
      file_manager,
      path,
      scene.save());

    logging::logf(
      "Scene saved to '{}'.",
      abs_path.string());

    return true;
}

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
#include "renderer/mesh_manager.h"
#include "renderer/render_device.h"
#include "renderer/renderer.h"
#include "scene/floor.h"
#include "scene/gear.h"
#include "scene/scene.h"
#include "scene/static_mesh.h"
#include "serialization/file.h"
#include "serialization/json/scene_loader.h"
#include "tasks/task_system.h"
#include "ui/imgui.h"
#include "application.h"
#include "file_manager.h"
#include "logging.h"
#include "runtime_asset_resolver.h"
#include "shader_factory.h"
#include "staged_data.h"
#include "viewport.h"

using task_system::TaskCancelledError;
using task_system::TaskExecutionContext;
using task_system::TaskGroupSnapshot;
using task_system::TaskHandle;
using task_system::TaskSnapshot;
using task_system::TaskSpec;
using task_system::TaskState;

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

struct SDLError final
: public std::runtime_error
{
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

void imgui_draw_viewport_panel(
  ResourceTracker& resource_tracker,
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

    if(viewport_texture == 0)
    {
        viewport_input.viewport_hovered = false;
        viewport_input.viewport_rect_valid = false;

        ImGui::End();
        return;
    }

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

    float text_y = 4.f;

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
        text_y += label_size.y + 4.f;

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
                view_index <= std::to_underlying(EditorCameraView::Orthographic);
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

    if(!resource_tracker.is_finished())
    {
        const swr::string status = swr::format(
          "Loading assets ({}/{})...",
          resource_tracker.pending_count(),
          resource_tracker.size());

        const ImVec2 text_pos = ImVec2{
          viewport_min.x + 8.0f,
          viewport_min.y + text_y};
        const ImVec2 text_size = ImGui::CalcTextSize(status.c_str());
        text_y += text_size.y + 4.f;

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
          viewport_min.y + text_y};
        const ImVec2 text_size = ImGui::CalcTextSize(status.c_str());
        text_y += text_size.y + 4.f;

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

    ImGui::End();
}

/*
 * Startup finalization.
 */

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
      "Loading scene...");
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

void ApplicationTaskSystemLogger::warning(std::string_view message) const
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
    if(scene_load_task_handle.valid())
    {
        return "Loading scene...";
    }

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
            logging::logf("Received SDL_EVENT_QUIT");

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
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void Application::render_frame()
{
    imgui::draw_main_dockspace(*this);
    imgui_draw_viewport_panel(
      resource_tracker,
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
    imgui::draw_memory_profiler_panel(
      render_device,
      material_manager,
      mesh_manager);

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
  ResourceTracker& resource_tracker,
  RenderDevice& render_device,
  Renderer& renderer,
  MaterialManager& material_manager,
  MeshManager& mesh_manager,
  Scene& scene,
  Viewport& viewport)
: title{title}
, log_device{log_device}
, file_manager{file_manager}
, task_system{task_system}
, resource_tracker{resource_tracker}
, render_device{render_device}
, renderer{renderer}
, material_manager{material_manager}
, mesh_manager{mesh_manager}
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

    /*
     * TODO Startup tasks can be added here.
     */
}

bool Application::is_startup_ready() const
{
    using namespace std::literals;

    // Check all futures for readiness.
    if(!startup_task_futures.empty())
    {
        for(const auto& startup_task_future: startup_task_futures)
        {
            if(!startup_task_future.valid()
               || startup_task_future.wait_for(0ms)
                    != std::future_status::ready)
            {
                return false;
            }
        }
    }

    if(scene_load_task_future.valid()
       && scene_load_task_future.wait_for(0ms)
            != std::future_status::ready)
    {
        return false;
    }

    // check resource tracker.
    return resource_tracker.is_finished();
}

bool Application::finish_startup_if_ready()
{
    if(!is_startup_ready())
    {
        return false;
    }

    // Check for loading errors.
    if(resource_tracker.has_failed())
    {
        on_startup_complete_error(
          "Resource failed to load.");
        throw std::runtime_error{
          "Resource failed to load."};
    }

    // All resources are loaded here, so we clear all.
    resource_tracker.clear();

    try
    {
        for(auto& startup_task_future: startup_task_futures)
        {
            if(startup_task_future.valid())
            {
                startup_task_future.get();
            }
        }

        if(scene_load_task_future.valid())
        {
            staged::StagedScene staged_scene = scene_load_task_future.get();
            scene.replace(std::move(staged_scene.scene));
            scene_load_task_handle = TaskHandle{};
            scene_load_task_future = std::future<staged::StagedScene>{};
            scene_load_task_error.reset();
        }

        /*
         * TODO Add code to finish startup here.
         */
    }
    catch(const std::exception& e)
    {
        on_startup_complete_error(e.what());
        startup_task_handles.clear();
        startup_task_futures.clear();
        startup_task_weights.clear();

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

void Application::update_scene_load_task()
{
    using namespace std::chrono_literals;

    if(!scene_load_task_future.valid())
    {
        return;
    }

    if(scene_load_task_future.wait_for(0ms)
       != std::future_status::ready)
    {
        return;
    }

    try
    {
        staged::StagedScene staged_scene = scene_load_task_future.get();
        scene.replace(std::move(staged_scene.scene));
        scene_load_task_error.reset();
    }
    catch(const TaskCancelledError&)
    {
        scene_load_task_error = "Scene load cancelled.";
        logging::warningf("Scene load task was cancelled.");
    }
    catch(const std::exception& e)
    {
        scene_load_task_error = e.what();
        logging::errorf(
          "Failed to load scene: {}",
          e.what());
    }

    scene_load_task_handle = TaskHandle{};
    scene_load_task_future = std::future<staged::StagedScene>{};
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

void Application::process_dirty_meshes()
{
    auto& objects_by_id = scene.get_objects_by_id();
    for(auto& object_id: scene.get_dirty_meshes())
    {
        auto it = objects_by_id.find(object_id);
        if(it == objects_by_id.end())
        {
            logging::warningf(
              "Deleted object with id '{}' requested reload. Skipping.",
              object_id.value);
            continue;
        }

        auto* mesh = reflect::try_cast<StaticMesh>(it->second);
        if(mesh == nullptr)
        {
            logging::warningf(
              "Object with id '{}' requested reload, but is not a StaticMesh.",
              object_id.value);
            continue;
        }

        auto& material_paths = mesh->get_material_paths();
        if(material_paths.empty())
        {
            logging::warningf(
              "Cannot create {} {}: No material set.",
              mesh->get_class()->name,
              mesh->get_name());
            continue;
        }
        if(material_paths.size() > 1)
        {
            logging::warningf(
              "Too many materials set for {} {} (got {}, expected 1). Picking first one.",
              mesh->get_class()->name,
              mesh->get_name(),
              material_paths.size());
        }

        const auto& material = mesh->get_material_ref();
        if(!material.has_value())
        {
            logging::errorf(
              "Cannot get material '{}' for {} {}.",
              material_paths[0].path.string(),
              mesh->get_class()->name,
              mesh->get_name());

            // TODO Currently this retries, but will spam the log if the material is not loaded.

            continue;
        }

        if(auto* gear = reflect::try_cast<Gear>(mesh))
        {
            if(const auto& pending = gear->get_pending_mesh_ref();
               pending.has_value())
            {
                const auto* lods = pending->try_get_lods();
                if(lods == nullptr)
                {
                    continue;
                }

                auto resolved_mesh = *pending;
                gear->set_mesh_ref(std::move(resolved_mesh));
                gear->set_lods(*lods);
                for(auto& lod: gear->get_lods())
                {
                    for(auto& section: lod.mesh_sections)
                    {
                        section.color = gear->get_color();
                    }
                }
                gear->clear_mesh_dirty();
                continue;
            }

            // Generate and upload new mesh.
            auto geom = gear->generate_mesh();

            auto params = Gear::create_gear_resources(
              mesh_manager,
              gear->get_name(),
              material.value(),
              gear->get_inner_radius(),
              gear->get_outer_radius(),
              gear->get_width(),
              gear->get_teeth(),
              gear->get_tooth_depth(),
              gear->get_color(),
              geom);

            gear->set_pending_mesh_ref(params.mesh);

            const auto* lods = params.mesh.try_get_lods();
            if(lods == nullptr)
            {
                continue;
            }

            gear->set_mesh_ref(params.mesh);
            gear->set_lods(*lods);
            for(auto& lod: gear->get_lods())
            {
                for(auto& section: lod.mesh_sections)
                {
                    section.color = gear->get_color();
                }
            }
        }
        else if(auto* floor = reflect::try_cast<Floor>(mesh))
        {
            if(const auto& pending = floor->get_pending_mesh_ref();
               pending.has_value())
            {
                const auto* lods = pending->try_get_lods();
                if(lods == nullptr)
                {
                    continue;
                }

                auto resolved_mesh = *pending;
                floor->set_mesh_ref(std::move(resolved_mesh));
                floor->set_lods(*lods);
                for(auto& lod: floor->get_lods())
                {
                    for(auto& section: lod.mesh_sections)
                    {
                        section.color = {1.f, 1.f, 1.f, 1.f};
                    }
                }
                floor->clear_mesh_dirty();
                continue;
            }

            const assets::AssetPath floor_path{
              swr::format(
                "floor://{}:{}",
                floor->get_half_extent(),
                floor->get_uv_repeat())};
            auto floor_material = material.value();
            auto floor_ref = mesh_manager.reload_sync(
              floor_path,
              {floor->generate_mesh()},
              floor_material);
            floor->set_pending_mesh_ref(floor_ref);

            const auto* lods = floor_ref.try_get_lods();
            if(lods == nullptr)
            {
                continue;
            }

            floor->set_mesh_ref(floor_ref);
            floor->set_lods(*lods);
            for(auto& lod: floor->get_lods())
            {
                for(auto& section: lod.mesh_sections)
                {
                    section.color = {1.f, 1.f, 1.f, 1.f};
                }
            }
        }
        else
        {
            /* StaticMesh. */

            auto& path = mesh->get_path();
            if(path.path.empty())
            {
                logging::warningf(
                  "No asset path for {} {}.",
                  mesh->get_class()->name,
                  mesh->get_name());
                continue;
            }

            auto mesh_ref = mesh_manager.try_get(path);
            if(!mesh_ref.has_value())
            {
                logging::errorf(
                  "Asset '{}' not found for {} {}.",
                  mesh->get_path().path.string(),
                  mesh->get_class()->name,
                  mesh->get_name());
                continue;
            }

            const auto* lods = mesh_ref.value().try_get_lods();
            if(lods == nullptr)
            {
                continue;
            }

            mesh->set_lods(*lods);
            mesh->set_mesh_ref(
              std::move(mesh_ref.value()));
        }

        mesh->clear_mesh_dirty();
    }

    // FIXME The code doesn't clear the still-dirty meshes (on purpose),
    //       but it's done by clear-all & re-insert.
    scene.clear_dirty_meshes();
    scene.for_each_object<StaticMesh>(
      [&](StaticMesh& mesh)
      {
          if(mesh.is_mesh_dirty())
          {
              scene.mark_mesh_dirty(mesh.get_object_id());
          }
      });
}

void Application::tick(float delta_time)
{
    /*
     * Reset per-frame memory.
     */

    memory::frame_bump().reset();
    memory::frame_arena().reset();

    /*
     * Process pending tasks from other systems.
     */

    // Release resources from a previous scene before creating replacement resources.
    // FIXME We don't really want to do this, since we could/should keep assets.
    //       But we cannot simply re-order the logic here, since processing first
    //       doesn't update deferred deletions, so we could end up in an inconsistent
    //       state with deleted-but-used assets.
    render_device.process_deferred_deletions();

    mesh_manager.process_pending();
    material_manager.process_pending();

    process_dirty_meshes();

    /*
     * Update scene and background tasks.
     */

    update_scene_load_task();
    update_runtime_test_task();

    /*
     * Input.
     */

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

    /*
     * Scene.
     */

    scene.tick(delta_time);
}

void Application::set_static_mesh_material(StaticMeshMaterial type)
{
    active_static_mesh_material = type;

    const auto material_path = [&]() -> assets::AssetPath
    {
        if(type == StaticMeshMaterial::ColorFlat)
        {
            return assets::AssetPath{"assets/materials/mesh/flat.json"};
        }
        else if(type == StaticMeshMaterial::ColorSmooth)
        {
            return assets::AssetPath{"assets/materials/mesh/smooth.json"};
        }
        else if(type == StaticMeshMaterial::PhongSmooth)
        {
            return assets::AssetPath{"assets/materials/mesh/phong.json"};
        }
        else if(type == StaticMeshMaterial::LitSmooth)
        {
            return assets::AssetPath{"assets/materials/mesh/lit.json"};
        }
        else
        {
            throw std::runtime_error{"Unknown mesh material."};
        }
    }();

    const MaterialRef material = [&]() -> MaterialRef
    {
        // Avoid filesystem access.
        auto cached_material = material_manager.get(material_path);
        if(cached_material.has_value())
        {
            return cached_material.value();
        }

        auto json = read_text_file(file_manager, material_path.path);
        return material_manager.load(material_path, json);
    }();

    for(auto& mesh: scene.objects_of<StaticMesh>())
    {
        if(mesh.get_class() != StaticMesh::static_class())
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

void Application::set_floor_material(FloorMaterial type)
{
    active_floor_material = type;

    const auto path = [&]() -> assets::AssetPath
    {
        if(type == FloorMaterial::TexturedFloor)
        {
            return assets::AssetPath{"assets/materials/floor/floor.json"};
        }
        else if(type == FloorMaterial::TexturedShinyFloor)
        {
            return assets::AssetPath{"assets/materials/floor/shiny_floor.json"};
        }
        else
        {
            throw std::runtime_error{"Unknown floor shader type."};
        }
    }();

    const MaterialRef material = [&]() -> MaterialRef
    {
        // Avoid filesystem access.
        auto cached_material = material_manager.get(path);
        if(cached_material.has_value())
        {
            return cached_material.value();
        }

        return material_manager.load(
          path,
          read_text_file(file_manager, path.path));
    }();

    for(auto& mesh: scene.objects_of<Floor>())
    {
        for(auto& lod: mesh.get_lods())
        {
            for(auto& section: lod.mesh_sections)
            {
                section.material = material;
            }
        }
    }
}

void Application::new_scene()
{
    destroy_scene();

    // The scene is empty and can be used.
}

void Application::destroy_scene()
{
    scene.clear();

    material_manager.prune();
    material_manager.get_texture_cache().prune();
    mesh_manager.clear();
}

bool Application::load_scene(
  const std::filesystem::path& path)
{
    if(scene_load_task_handle.valid())
    {
        scene_load_task_handle.cancel();
        scene_load_task_handle.wait();
        scene_load_task_handle = TaskHandle{};
        scene_load_task_future = std::future<staged::StagedScene>{};
        scene_load_task_error.reset();
    }

    resource_tracker.clear();

    logging::logf(
      "Loading scene '{}'...",
      path.string());

    new_scene();

    try
    {
        auto contents = read_text_file(file_manager, path);

        auto submission = task_system.submit(
          [this,
           path = std::filesystem::path{path},
           contents = swr::string{std::move(contents)}](
            task_system::TaskExecutionContext& context) mutable -> staged::StagedScene
          {
              if(context.is_cancel_requested())
              {
                  throw task_system::TaskCancelledError{};
              }

              staged::StagedScene staged_scene;
              RuntimeAssetResolver resolver{
                file_manager,
                material_manager,
                mesh_manager};
              serial::json::JsonSceneLoader loader{resolver};
              loader.load(staged_scene.scene, contents);

              if(context.is_cancel_requested())
              {
                  throw task_system::TaskCancelledError{};
              }

              logging::logf(
                "Loaded scene '{}' on worker thread.",
                path.string());

              return staged_scene;
          });

        scene_load_task_handle = submission.handle;
        scene_load_task_future = std::move(submission.future);
    }
    catch(const std::runtime_error& e)
    {
        logging::errorf(
          "Failed to load scene from '{}': {}",
          path.string(),
          e.what());
        scene_load_task_handle = TaskHandle{};
        scene_load_task_future = std::future<staged::StagedScene>{};
        scene_load_task_error = e.what();
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

void Application::reset()
{
    viewport.reset_editor_camera();
    viewport.use_local_camera();
    viewport.set_editor_camera_view(EditorCameraView::Perspective);
    viewport.set_display_settings(ViewportDisplaySettings{});
    viewport.set_overlay_settings(ViewportOverlaySettings{});

    new_scene();
}

/**
 * Software Rasterizer Playground.
 *
 * ImGui support.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "reflection/class_info.h"

/*
 * Forward declarations.
 */

struct ImGuiIO;
struct ImFont;
class RenderDevice;
class Renderer;
class Scene;
class Object;
class Viewport;
class Application;

namespace logging
{
class BufferedLogDevice;
}    // namespace logging

namespace imgui
{

/** UI state. */
struct State
{
    /** Currently selected scene object in the scene inspector. */
    Object* selected_scene_object = nullptr;

    /** Currently selected class in the class inspector. */
    const reflect::ClassInfo* selected_class = nullptr;
};

/** Set up ImGui. */
bool init(
  SDL_Window* window,
  SDL_GLContext context);

/** Shut down ImGui. */
void shutdown();

/** Smaller UI font for compact controls. */
ImFont* get_small_ui_font();

/** Monospace font for console/log output. */
ImFont* get_console_monospace_font();

void draw_main_dockspace(Application& app);

void draw_console_panel(
  logging::BufferedLogDevice& log_device);

void draw_tools_panel(
  class Application& app,
  RenderDevice& render_device,
  Viewport& viewport,
  Scene& scene,
  Renderer& renderer,
  int frame_index,
  float pixel_density);

void draw_profiler_panel(
  Renderer& renderer);

void draw_memory_profiler_panel();

bool check_and_clear_sorting_benchmark_request();

void draw_scene_inspector_panel(
  State& ui_state,
  Scene& scene,
  RenderDevice& render_device);

void draw_class_inspector_panel(
  State& ui_state);

}    // namespace imgui

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

#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "reflection/class_info.h"

/*
 * Forward declarations.
 */

struct ImGuiIO;
class RenderDevice;
class Renderer;
class Scene;
class Object;
class Viewport;

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

void draw_main_dockspace(
  bool& running);

void draw_console_panel(
  std::vector<std::string>& log_lines);

void draw_tools_panel(
  RenderDevice& render_device,
  Viewport& viewport,
  Scene& scene,
  Renderer& renderer,
  int frame_index,
  float pixel_density,
  const ImGuiIO& io);

void draw_scene_inspector_panel(
  State& ui_state,
  Scene& scene);

void draw_class_inspector_panel(
  State& ui_state);

}    // namespace imgui

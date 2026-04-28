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

struct ImGuiIO;
class RenderDevice;
class Renderer;
class Scene;
class Object;
struct Viewport;

/** Set up ImGui. */
bool imgui_init(
  SDL_Window* window,
  SDL_GLContext context);

/** Shut down ImGui. */
void imgui_shutdown();

void imgui_draw_main_dockspace(bool& running);

void imgui_draw_console_panel(std::vector<std::string>& log_lines);

void imgui_draw_tools_panel(
  RenderDevice& render_device,
  Viewport& viewport,
  Scene& scene,
  Renderer& renderer,
  int frame_index,
  float pixel_density,
  const ImGuiIO& io);

void imgui_draw_scene_inspector_panel(Scene& scene);
void imgui_draw_class_inspector_panel();

Object* imgui_get_selected_object() noexcept;
void imgui_set_selected_object(Object* object) noexcept;
void imgui_clear_selected_object() noexcept;

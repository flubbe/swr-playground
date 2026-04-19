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

/** Set up ImGui. */
bool imgui_init(
  SDL_Window* window,
  SDL_GLContext context);

/** Shut down ImGui. */
void imgui_shutdown();

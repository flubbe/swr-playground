/**
 * Software Rasterizer Playground.
 *
 * Basic platform support.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <thread>

#include <SDL3/SDL.h>

#include "logging.h"

/** default SDL log device (set in global_initialize and used in global_shutdown) */
static SDL_LogOutputFunction default_sdl_log{nullptr};

/** map SDL output to log device. */
void sdl_log(
  [[maybe_unused]] void* userdata,
  [[maybe_unused]] int category,
  [[maybe_unused]] SDL_LogPriority priority,
  const char* message)
{
    logging::logf("{}", message);
}

bool platform_init(
  [[maybe_unused]] int argc,
  [[maybe_unused]] char* argv[])
{
    logging::logf(
      "std::thread::hardware_concurrency: {}",
      std::thread::hardware_concurrency());

    /* map SDL output to log. */
    SDL_GetLogOutputFunction(&default_sdl_log, nullptr);
    SDL_SetLogOutputFunction(&sdl_log, nullptr);

    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        logging::errorf("SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    // Attributes should be set before context creation.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    return true;
}

/** Global platform shutdown. */
void platform_shutdown()
{
    SDL_Quit();
}

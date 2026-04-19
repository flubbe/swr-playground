/**
 * Software Rasterizer Playground.
 *
 * Basic platform support.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <print>

#include <SDL3/SDL.h>

bool platform_init(
  [[maybe_unused]] int argc,
  [[maybe_unused]] char* argv[])
{
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::println(stderr, "SDL_Init failed: {}", SDL_GetError());
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

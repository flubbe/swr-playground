/**
 * Software Rasterizer Playground.
 *
 * Application startup.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <gsl/gsl>

#include "scene/scene.h"
#include "application.h"
#include "renderdevice.h"
#include "renderer.h"
#include "shader_cache.h"
#include "platform.h"
#include "viewport.h"

int main(int argc, char* argv[])
{
    if(!platform_init(argc, argv))
    {
        return EXIT_FAILURE;
    }

    ReflectionSystem::process_pending_registrations();

    const auto shutdown = gsl::finally([]() -> void
                                       { platform_shutdown(); });

    Application app{
      "SWR Playground"};

    RenderDevice render_device{640, 480};
    ShaderCache shader_cache;
    Renderer renderer{render_device};

    Scene scene;
    Viewport viewport;

    app.initialize(
      render_device,
      shader_cache,
      renderer,
      scene,
      viewport);

    app.run();

    return 0;
}
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
#include "logging.h"
#include "renderdevice.h"
#include "renderer.h"
#include "platform.h"
#include "viewport.h"

int main(int argc, char* argv[])
{
    logging::BufferedLogDevice log_device;
    logging::log_init(&log_device);

    const auto log_shutdown = gsl::finally([]() -> void
                                           { logging::log_shutdown(); });

    if(!platform_init(argc, argv))
    {
        return EXIT_FAILURE;
    }

    reflect::ReflectionSystem::allow_auto_registration(false);
    reflect::ReflectionSystem::process_pending_registrations();

    const auto shutdown = gsl::finally([]() -> void
                                       { platform_shutdown(); });

    Application app{
      "SWR Playground",
      log_device};

    RenderDevice render_device{640, 480};
    Renderer renderer{render_device};

    Scene scene;
    Viewport viewport;

    app.initialize(
      render_device,
      renderer,
      scene,
      viewport);

    app.run();

    return 0;
}

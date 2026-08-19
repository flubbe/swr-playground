/**
 * Software Rasterizer Playground.
 *
 * Application startup.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string_view>

#include <gsl/gsl>

#include "containers/memory.h"
#include "renderer/material_manager.h"
#include "renderer/mesh_manager.h"
#include "renderer/render_device.h"
#include "renderer/renderer.h"
#include "scene/scene.h"
#include "memory/manager.h"
#include "application.h"
#include "file_manager.h"
#include "logging.h"
#include "main_loop.h"
#include "platform.h"
#include "shader_cache.h"
#include "texture_cache.h"
#include "viewport.h"

namespace
{

/** Initial framebuffer width. */
constexpr int initial_framebuffer_width = 640;

/** Initial framebuffer height. */
constexpr int initial_framebuffer_height = 480;

/** Default persistent log file path. */
const std::filesystem::path default_log_path{"logs/swr_playground.log"};

[[nodiscard]]
std::filesystem::path resolve_log_path(int argc, char* argv[])
{
    for(int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        if(arg.starts_with("--log-file="))
        {
            const std::string_view value = arg.substr(std::string_view{"--log-file="}.size());
            if(value.empty())
            {
                throw std::invalid_argument("--log-file requires a non-empty path.");
            }

            return std::filesystem::path{value};
        }

        if(arg == "--log-file")
        {
            if(i + 1 >= argc)
            {
                throw std::invalid_argument("--log-file requires a path argument.");
            }

            return std::filesystem::path{argv[++i]};
        }
    }

    const char* configured_path = std::getenv("SWR_PLAYGROUND_LOG_PATH");
    if(configured_path != nullptr && configured_path[0] != '\0')
    {
        return configured_path;
    }

    return default_log_path;
}

} /* namespace*/

int main(int argc, char* argv[])
{
    memory::initialize();
    const auto memory_shutdown =
      gsl::finally([]() -> void
                   { memory::shutdown(); });
    // TODO It would be nice to (automatically) log memory statistics, but logging is shut down earlier.

    const auto log_shutdown =
      gsl::finally([]() -> void
                   { logging::shutdown(); });

    FileManager file_manager;
    file_manager.add_search_path(".");
    file_manager.set_writable_root(".");

    logging::FileLogDevice log_device{
      file_manager.resolve_write(
        resolve_log_path(argc, argv)),
      logging::FileLogDeviceOptions{
        .overflow_policy = logging::OverflowPolicy::DropNewest,
      }};
    logging::initialize(&log_device);

#ifndef DEBUG
    try
    {
#endif
        if(!platform_init(argc, argv))
        {
            return EXIT_FAILURE;
        }

        const auto shutdown = gsl::finally([]() -> void
                                           { platform_shutdown(); });

        // This seems to be the earliest point where we can easily display
        // the splash screen. It needs logging to be set up in case of errors,
        // and the platform initialization also sets up TTF support.
        auto splash_screen = swr::make_unique<SplashScreen>(
          640, 480);
        splash_screen->set_status("Loading...");

        reflect::ReflectionSystem::allow_auto_registration(false);
        reflect::ReflectionSystem::process_pending_registrations();

        ApplicationTaskSystemLogger task_system_logger{log_device};
        task_system::TaskSystem task_system{
          std::thread::hardware_concurrency(),
          task_system_logger};

        RenderDevice render_device{
          initial_framebuffer_width,
          initial_framebuffer_height};
        Renderer renderer{render_device};

        ShaderCache shader_cache{render_device};
        TextureCache texture_cache{render_device};
        MaterialManager material_manager{
          task_system,
          render_device,
          shader_cache,
          renderer.get_shader_factory(),
          texture_cache};
        MeshManager mesh_manager{
          task_system,
          render_device};

        Scene scene;
        Viewport viewport;

        Application app{
          "SWR Playground",
          log_device,
          file_manager,
          task_system,
          render_device,
          renderer,
          material_manager,
          mesh_manager,
          scene,
          viewport};

        // Set up the main loop and exit the splash screen just before entering.
        MainLoop main_loop{*splash_screen, app};
        if(!main_loop.run_startup())
        {
            return EXIT_FAILURE;
        }

        splash_screen.reset();
        main_loop.run();

        // TODO cancel tasks and wait for all before resource cleanup.

#ifndef DEBUG
    }
    catch(const std::exception& e)
    {
        logging::fatalf("{}", e.what());
        return EXIT_FAILURE;
    }
    catch(...)
    {
        logging::fatalf("Terminating after uncaught exception.");
        return EXIT_FAILURE;
    }
#endif

    return 0;
}

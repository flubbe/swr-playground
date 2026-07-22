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

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string_view>

#include "scene/scene.h"
#include "memory/manager.h"
#include "application.h"
#include "logging.h"
#include "main_loop.h"
#include "renderdevice.h"
#include "renderer.h"
#include "platform.h"
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

    auto memstats = memory::stats();
    std::println("Allocs [mem init]: {}", memstats.allocate_calls);

    const auto log_shutdown =
      gsl::finally([]() -> void
                   { logging::shutdown(); });

    logging::FileLogDevice log_device{
      resolve_log_path(argc, argv),
      logging::FileLogDeviceOptions{
        .overflow_policy = logging::OverflowPolicy::DropNewest,
      }};
    logging::initialize(&log_device);

    memstats = memory::stats();
    std::println("Allocs [logging init]: {}", memstats.allocate_calls);

    try
    {
        if(!platform_init(argc, argv))
        {
            return EXIT_FAILURE;
        }

        const auto shutdown = gsl::finally([]() -> void
                                           { platform_shutdown(); });

        memstats = memory::stats();
        std::println("Allocs [platform init]: {}", memstats.allocate_calls);

        // This seems to be the earliest point where we can easily display
        // the splash screen. It needs logging to be set up in case of errors,
        // and the platform initialization also sets up TTF support.
        auto splash_screen = std::make_unique<SplashScreen>(
          640, 480);
        splash_screen->set_status("Loading...");

        reflect::ReflectionSystem::allow_auto_registration(false);
        reflect::ReflectionSystem::process_pending_registrations();

        memstats = memory::stats();
        std::println("Allocs [reflection init]: {}", memstats.allocate_calls);

        RenderDevice render_device{
          initial_framebuffer_width,
          initial_framebuffer_height};
        Renderer renderer{render_device};

        Scene scene;
        Viewport viewport;

        Application app{
          "SWR Playground",
          log_device,
          render_device,
          renderer,
          scene,
          viewport,
          std::thread::hardware_concurrency()};

        memstats = memory::stats();
        std::println("Allocs [app init]: {}", memstats.allocate_calls);

        memory::print_histogram();

        // Set up the main loop and exit the splash screen just before entering.
        MainLoop main_loop{*splash_screen, app};
        if(!main_loop.run_startup())
        {
            return EXIT_FAILURE;
        }

        splash_screen.reset();

        memstats = memory::stats();
        std::println("Allocs [main loop]: {}", memstats.allocate_calls);

        memory::print_histogram();

        main_loop.run();
    }
    catch(const std::exception& e)
    {
        logging::errorf("Error: {}", e.what());
        return EXIT_FAILURE;
    }
    catch(...)
    {
        logging::errorf("Terminating after uncaught exception.");
        return EXIT_FAILURE;
    }

    return 0;
}

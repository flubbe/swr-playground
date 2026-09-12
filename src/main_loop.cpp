/**
 * Software Rasterizer Playground.
 *
 * Main loop orchestration.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <chrono>

#include "application.h"
#include "main_loop.h"

MainLoop::MainLoop(
  SplashScreen& splash_screen,
  Application& application) noexcept
: splash_screen{splash_screen}
, application{application}
{
}

void MainLoop::run()
{
    auto last_update_time = std::chrono::steady_clock::now();
    bool running = true;

    // ensure the application window is shown.
    application.show_window();

    while(running)
    {
        running = application.pump_messages();

        auto now = std::chrono::steady_clock::now();
        float delta_time =
          std::chrono::duration<float>(now - last_update_time).count();
        last_update_time = now;

        application.tick(delta_time);

        application.prepare_frame();
        application.render_frame();
    }
}

bool MainLoop::run_startup()
{
    using namespace std::literals;

    bool running = true;

    constexpr auto frame_time = 100ms;
    auto next_frame_time = std::chrono::steady_clock::now();

    application.begin_startup();
    while(running
          && !application.finish_startup_if_ready())
    {
        running = application.pump_messages();

        // Update startup progress.
        // We naively always update, even if there's no change.
        splash_screen.set_status(
          application.get_startup_status());

        // wait for next frame.
        next_frame_time += frame_time;

        if(auto now = std::chrono::steady_clock::now();
           next_frame_time > now)
        {
            std::this_thread::sleep_until(
              next_frame_time);
        }
        else
        {
            // We've fallen behind; don't try to catch up.
            next_frame_time = now;
        }
    }

    return running;
}

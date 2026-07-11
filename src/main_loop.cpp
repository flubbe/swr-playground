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

MainLoop::MainLoop(Application& application) noexcept
: application{application}
{
}

void MainLoop::run()
{
    application.begin_startup();

    if(!run_startup())
    {
        return;
    }

    run_main();
}

bool MainLoop::run_startup()
{
    bool running = true;

    while(running)
    {
        running = application.pump_messages();

        application.prepare_frame();

        if(application.finish_startup_if_ready())
        {
            break;
        }

        application.render_loading_frame();
    }

    return running;
}

void MainLoop::run_main()
{
    auto last_update_time = std::chrono::steady_clock::now();

    bool running = true;
    while(running)
    {
        running = application.pump_messages();

        application.prepare_frame();

        auto now = std::chrono::steady_clock::now();
        float delta_time =
          std::chrono::duration<float>(now - last_update_time).count();
        last_update_time = now;

        application.tick(delta_time);

        application.render_main_frame();
    }
}

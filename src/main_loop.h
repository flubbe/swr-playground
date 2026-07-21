/**
 * Software Rasterizer Playground.
 *
 * Main loop orchestration.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

class Application;
class SplashScreen;

/**
 * Orchestrates the application's main loop and startup sequence.
 *
 * Manages the application lifetime by initializing the startup async task,
 * running the startup phase (loading with progress UI), followed by the main
 * loop (rendering and event processing). Platform-specific functions (SDL, ImGui)
 * are delegated to the Application class.
 */
class MainLoop
{
public:
    /**
     * Constructor.
     *
     * @param application Reference to the application object to orchestrate.
     */
    explicit MainLoop(
      SplashScreen& splash_screen,
      Application& application) noexcept;

    /**
     * Runs the startup phase with a loading screen.
     *
     * Waits for the startup future to complete while displaying a loading UI.
     * Finalizes the scene once loading is complete.
     *
     * @returns true if startup was successful, false if aborted.
     */
    bool run_startup();

    /**
     * Runs main loop until the application quits.
     */
    void run();

private:
    SplashScreen& splash_screen;
    Application& application;
};

/**
 * Software Rasterizer Playground.
 *
 * Main application.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <array>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "logging.h"
#include "tasks/task_system.h"
#include "ui/imgui.h"

class RenderDevice;
class Renderer;
class Scene;
struct PreparedStartupScene;
class Viewport;
class MainLoop;

struct ViewportInputState
{
    bool viewport_hovered{false};
    bool viewport_rect_valid{false};
    float viewport_min_x{0.f};
    float viewport_min_y{0.f};
    float viewport_max_x{0.f};
    float viewport_max_y{0.f};
    float mouse_delta_x{0.f};
    float mouse_delta_y{0.f};
    float mouse_wheel_delta{0.f};
};

enum class StaticMeshShaderType
{
    ColorFlat,
    ColorSmooth,
    PhongSmooth,
    LitSmooth
};

enum class FloorShaderType
{
    TexturedFloor,
    TexturedShinyFloor
};

namespace logging
{
class BufferedLogDevice;
}    // namespace logging

class ApplicationTaskSystemLogger final
: public task_system::TaskLogger
{
    const logging::Logger logger;

public:
    explicit ApplicationTaskSystemLogger(
      logging::LogDevice& log_device);

    void log(std::string_view message) const override;
    void warn(std::string_view message) const override;
    void error(std::string_view message) const override;
};

class Application
{
    std::string title;
    logging::BufferedLogDevice& log_device;

    SDL_Window* window{nullptr};
    SDL_GLContext gl_context{nullptr};

    RenderDevice& render_device;
    Renderer& renderer;

    Scene& scene;
    Viewport& viewport;

    ApplicationTaskSystemLogger task_system_logger;
    task_system::TaskSystem task_system;

    int window_w{0};
    int window_h{0};
    int pixel_w{0};
    int pixel_h{0};
    float pixel_density{1.f};
    float display_scale{1.f};

    GLuint viewport_texture = 0;

    /**
     * Currently active shader type for static meshes.
     *
     * FIXME Likely not the correct place, but convenient for experimenting.
     */
    StaticMeshShaderType active_static_mesh_shader{StaticMeshShaderType::LitSmooth};
    FloorShaderType active_floor_shader{FloorShaderType::TexturedFloor};
    std::array<std::uint32_t, 2> floor_texture_handles{};
    bool has_floor_textures{false};

    ViewportInputState viewport_input{};
    bool viewport_mouse_captured{false};
    bool viewport_mouse_restore_valid{false};
    float viewport_mouse_restore_x{0.f};
    float viewport_mouse_restore_y{0.f};
    bool prev_space_pressed{false};

    // Startup task state (parallel submissions aggregated by the main thread).
    std::shared_ptr<PreparedStartupScene> startup_scene;
    std::vector<task_system::TaskHandle> startup_task_handles;
    std::vector<std::future<void>> startup_task_futures;
    std::vector<float> startup_task_weights;

    // Runtime loader test task state (Debug -> Test tasks).
    task_system::TaskHandle runtime_test_task_handle;
    std::future<void> runtime_test_task_future;
    std::optional<std::string> runtime_test_task_error;
    bool runtime_test_modal_open{false};

    // Frame state for rendering
    int frame_index{0};
    imgui::State ui_state;

    // Startup error tracking
    std::optional<std::string> startup_error;

    // Temporary running flag for viewport rendering (can be set to false by viewport panel on error)
    mutable bool viewport_panel_running{true};

    void set_viewport_mouse_capture(bool enabled);
    void update_viewport_mouse_capture();

protected:
    void setup_viewport();

private:
    friend class MainLoop;

    /**
     * Pumps SDL messages and updates input state.
     *
     * @returns false if quit was requested, true otherwise.
     */
    bool pump_messages();

    /**
     * Prepares for a new frame by creating ImGui contexts.
     */
    void prepare_frame();

    /**
     * Renders the main application frame (viewport, panels, etc).
     */
    void render_main_frame();

    /**
     * Renders the loading screen frame.
     */
    void render_loading_frame();

    /**
     * Called when startup completes successfully.
     *
     * @param prepared_scene The prepared startup scene.
     */
    void on_startup_complete(const PreparedStartupScene& prepared_scene);

    /**
     * Called when startup encounters an error.
     *
     * @param error_message The error message.
     */
    void on_startup_complete_error(const std::string& error_message);

    /**
     * Starts the async startup task.
     */
    void begin_startup();

    /**
     * Returns true when the startup worker has completed.
     */
    [[nodiscard]]
    bool is_startup_ready() const;

    /**
     * Finalizes startup if the async worker has completed.
     *
     * @returns true if startup completed successfully and main loop can start.
     */
    bool finish_startup_if_ready();

    /** Cancel async startup tasks and wait for cancellation. */
    void cancel_startup();

    /** Poll and finalize runtime test task completion. */
    void update_runtime_test_task();

    /** Render modal loading popup for runtime test tasks. */
    void draw_runtime_test_modal();

public:
    void tick(float delta_time);

    /** Start async runtime test tasks from Debug menu. */
    void start_debug_test_tasks();

    /** Return whether runtime test tasks are currently running. */
    [[nodiscard]]
    bool is_debug_test_tasks_running() const noexcept;

public:
    Application(
      std::string_view title,
      logging::BufferedLogDevice& log_device,
      RenderDevice& render_device,
      Renderer& renderer,
      Scene& scene,
      Viewport& viewport,
      std::size_t thread_pool_workers);

    ~Application();

    // FIXME Likely not the correct place, but convenient for experimenting.
    void set_static_mesh_shader(StaticMeshShaderType type);

    // FIXME Likely not the correct place, but convenient for experimenting.
    void set_floor_shader(FloorShaderType type);

    // FIXME Likely not the correct place, but convenient for experimenting.
    StaticMeshShaderType get_static_mesh_shader() const noexcept
    {
        return active_static_mesh_shader;
    }

    FloorShaderType get_floor_shader() const noexcept
    {
        return active_floor_shader;
    }
};

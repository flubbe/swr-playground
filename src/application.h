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

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

class RenderDevice;
class Renderer;
class Scene;
class Viewport;

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

    void set_viewport_mouse_capture(bool enabled);
    void update_viewport_mouse_capture();

protected:
    void setup_scene();
    void setup_viewport();
    void tick(float delta_time);

public:
    Application(
      std::string_view title,
      logging::BufferedLogDevice& log_device,
      RenderDevice& render_device,
      Renderer& renderer,
      Scene& scene,
      Viewport& viewport);

    ~Application();

    void run();

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

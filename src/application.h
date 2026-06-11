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

#include <array>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "ml/all.h"

class Camera;
class Gear;
class Object;
class RenderDevice;
class Renderer;
class Scene;
class ShaderCache;
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

struct ViewportCameraControllerState
{
    ml::vec3 position{0.f, 0.f, 40.f};
    ml::vec3 orbit_target{0.f, 0.f, 0.f};
    float orbit_distance{40.f};
    float pitch_radians{ml::to_radians(20.f)};
    float yaw_radians{ml::to_radians(30.f)};
};

namespace shader
{
class ColorFlat;
class ColorSmooth;
class PhongSmooth;
}    // namespace shader

enum class StaticMeshShaderType
{
    ColorFlat,
    ColorSmooth,
    PhongSmooth,
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

    RenderDevice* render_device{nullptr};
    Renderer* renderer{nullptr};

    Scene* scene{nullptr};
    Viewport* viewport{nullptr};

    bool initialized{false};

    int window_w{0};
    int window_h{0};
    int pixel_w{0};
    int pixel_h{0};
    float pixel_density{1.f};
    float display_scale{1.f};

    GLuint viewport_texture = 0;

    // demo scene.
    std::array<Gear*, 3> gear_objs = {nullptr, nullptr, nullptr};

    /**
     * Currently active shader type for static meshes.
     *
     * FIXME Likely not the correct place, but convenient for experimenting.
     */
    StaticMeshShaderType active_static_mesh_shader{StaticMeshShaderType::PhongSmooth};

    ViewportInputState viewport_input{};
    ViewportCameraControllerState viewport_camera_controller{};
    bool viewport_mouse_captured{false};

    void set_viewport_mouse_capture(bool enabled);
    void update_viewport_mouse_capture();

protected:
    void setup_scene();
    void setup_viewport();

public:
    Application(
      std::string_view title,
      logging::BufferedLogDevice& log_device);

    ~Application();

    void initialize(
      RenderDevice& render_device,
      Renderer& renderer,
      Scene& scene,
      Viewport& viewport);

    void run();

    void tick(float delta_time);

    void reset_viewport_camera();

    // FIXME Likely not the correct place, but convenient for experimenting.
    void set_static_mesh_shader(StaticMeshShaderType type);

    // FIXME Likely not the correct place, but convenient for experimenting.
    StaticMeshShaderType get_static_mesh_shader() const noexcept
    {
        return active_static_mesh_shader;
    }
};

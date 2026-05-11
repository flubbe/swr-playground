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
    float mouse_delta_x{0.f};
    float mouse_delta_y{0.f};
};

struct ViewportCameraControllerState
{
    ml::vec3 position{0.f, 0.f, 40.f};
    float pitch_radians{ml::to_radians(20.f)};
    float yaw_radians{ml::to_radians(30.f)};
};

namespace shader
{
class ColorFlat;
class ColorSmooth;
}    // namespace shader

class Application
{
    std::string title;

    SDL_Window* window{nullptr};
    SDL_GLContext gl_context{nullptr};

    RenderDevice* render_device{nullptr};
    ShaderCache* shader_cache{nullptr};
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

    ViewportInputState viewport_input{};
    ViewportCameraControllerState viewport_camera_controller{};

protected:
    void setup_scene();
    void setup_viewport();

public:
    explicit Application(
      std::string_view title);

    ~Application();

    void initialize(
      RenderDevice& render_device,
      ShaderCache& shader_cache,
      Renderer& renderer,
      Scene& scene,
      Viewport& viewport);

    void run();

    void tick(float delta_time);
};

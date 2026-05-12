/**
 * Software Rasterizer Playground.
 *
 * A renderer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "shader_cache.h"

class Scene;
class Camera;
class Viewport;
struct ViewportDisplaySettings;
class RenderDevice;

class Renderer final
{
    RenderDevice& device;
    ShaderCache shader_cache;

    float render_time{0.f};

    /*
     * Viewport overlays.
     */

    RenderData overlay_grid;

    void create_grid_mesh();

    /*
     * Rendering functions.
     */

    void render_scene(
      const Scene& scene,
      const Camera& camera,
      const ViewportDisplaySettings& display_settings);

    void render_grid(
      const Camera& camera);

public:
    explicit Renderer(
      RenderDevice& device)
    : device{device}
    {
        create_grid_mesh();
    }

    [[nodiscard]]
    ShaderCache& get_shader_cache()
    {
        return shader_cache;
    }

    [[nodiscard]]
    float get_render_time() const noexcept
    {
        return render_time;
    }

    void render(
      const Scene& scene,
      const Viewport& viewport);
};

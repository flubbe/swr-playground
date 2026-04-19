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

class Scene;
class Camera;
struct Viewport;
class RenderDevice;

class Renderer
{
    RenderDevice& device;

    float render_time{0.f};

public:
    explicit Renderer(
      RenderDevice& device)
    : device{device}
    {
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
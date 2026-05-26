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

#include <array>
#include <cstddef>

#include "render_types.h"
#include "shader_cache.h"

class Scene;
class Camera;
class Viewport;
struct ViewportDisplaySettings;
class RenderDevice;

struct RendererStats
{
    static constexpr std::size_t tracked_lod_count{3};

    std::size_t static_meshes{0};
    std::size_t mesh_sections_drawn{0};
    std::size_t mesh_sections_culled{0};
    std::size_t triangles_submitted{0};
    std::size_t triangles_frustum_culled{0};
    std::array<std::size_t, tracked_lod_count> static_mesh_lods_selected{};
    std::size_t static_mesh_lods_selected_overflow{0};
};

class Renderer final
{
    RenderDevice& device;
    ShaderCache shader_cache;

    float render_time{0.f};
    RendererStats render_stats;

    /*
     * Viewport overlays.
     */

    MeshSection overlay_grid;

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

    [[nodiscard]]
    const RendererStats& get_stats() const noexcept
    {
        return render_stats;
    }

    void render(
      const Scene& scene,
      const Viewport& viewport);
};

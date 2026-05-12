/**
 * Software Rasterizer Playground.
 *
 * A renderer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <chrono>

#include "renderdevice.h"
#include "renderer.h"
#include "scene/scene.h"
#include "viewport.h"

void Renderer::render_scene(
  const Scene& scene,
  const Camera& camera,
  const ViewportDisplaySettings& display_settings)
{
    device.bind_rasterizer_state({.wireframe = display_settings.wireframe,
                                  .cull_face = display_settings.cull_face});

    auto view = camera.get_transform();
    auto projection = camera.get_projection_matrix();
    auto light_dir = ml::matrices::translation(view.rows[0].w, view.rows[1].w, view.rows[2].w)
                     * scene.get_light().position;

    for(const auto& obj: scene.get_objects())
    {
        if(!obj->is_drawable())
        {
            continue;
        }

        auto obj_view = view * obj->get_transform();

        const auto& meshes = obj->get_meshes();
        for(const auto& mesh: meshes)
        {
            device.bind_material(mesh.material_handle);
            device.bind_uniforms({.proj = projection,
                                  .view = obj_view,
                                  .light_dir = light_dir}

            );
            device.draw_mesh(mesh.mesh_handle);
        }
    }
}

void Renderer::render(
  const Scene& scene,
  const Viewport& viewport)
{
    auto render_start_time = std::chrono::steady_clock::now();

    const auto& display_settings = viewport.get_display_settings();

    device.begin_frame();

    const Camera& camera = viewport.get_camera(scene);

    render_scene(
      scene,
      camera,
      display_settings);

    device.end_frame();

    render_time = std::chrono::duration<float>(
                    std::chrono::steady_clock::now() - render_start_time)
                    .count();
}

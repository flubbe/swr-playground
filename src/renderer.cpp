/**
 * Software Rasterizer Playground.
 *
 * A renderer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "renderdevice.h"
#include "renderer.h"
#include "scene/scene.h"
#include "viewport.h"

void Renderer::render(
  const Scene& scene,
  const Viewport& viewport)
{
    auto render_start_time = std::chrono::steady_clock::now();

    device.begin_frame(
      viewport.draw_params.wireframe,
      viewport.draw_params.cull_face);

    auto view = viewport.camera.get_transform();
    for(auto& obj: scene.get_objects())
    {
        if(!obj->is_drawable())
        {
            continue;
        }

        auto light_dir = view * scene.get_light().position;
        auto obj_view = view * obj->get_transform();

        const auto& meshes = obj->get_meshes();
        for(const auto& mesh: meshes)
        {
            device.bind_material(mesh.material_handle);
            device.bind_uniforms({.proj = viewport.camera.get_projection_matrix(),
                                  .view = obj_view,
                                  .light_dir = light_dir}

            );
            device.draw_mesh(mesh.mesh_handle);
        }
    }

    device.end_frame();

    render_time = std::chrono::duration<float>(
                    std::chrono::steady_clock::now() - render_start_time)
                    .count();
}
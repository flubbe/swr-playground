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

void Renderer::render(
  Scene& scene,
  Viewport& viewport)
{
    auto render_start_time = std::chrono::steady_clock::now();

    device.begin_frame(
      viewport.draw_params.wireframe,
      viewport.draw_params.cull_face);

    Camera* active_camera = &viewport.local_camera;
    if(viewport.camera_source == ViewportCameraSource::SceneCamera)
    {
        if(Camera* scene_camera = scene.find_camera(viewport.scene_camera_id))
        {
            active_camera = scene_camera;
        }
    }

    active_camera->update_projection_matrix(viewport.get_aspect_ratio());
    auto view = active_camera->get_transform();
    auto projection = active_camera->get_projection_matrix();
    auto light_dir = ml::matrices::translation(view.rows[0].w, view.rows[1].w, view.rows[2].w) * scene.get_light().position;

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

    device.end_frame();

    render_time = std::chrono::duration<float>(
                    std::chrono::steady_clock::now() - render_start_time)
                    .count();
}

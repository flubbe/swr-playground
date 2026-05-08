/**
 * Software Rasterizer Playground.
 *
 * tool panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <imgui.h>

#include "scene/scene.h"
#include "renderdevice.h"
#include "renderer.h"
#include "viewport.h"

namespace imgui
{

void draw_tools_panel(
  RenderDevice& render_device,
  Viewport& viewport,
  Scene& scene,
  Renderer& renderer,
  int frame_index,
  float pixel_density,
  const ImGuiIO& io)
{
    ImGui::Begin("Tools");

    if(ImGui::CollapsingHeader(
         "Viewport",
         ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text(
          "Framebuffer: %d x %d px",
          render_device.get_width(),
          render_device.get_height());
        ImGui::Text("Window pixel density: %.2f", pixel_density);
        ImGui::Text("Frame: %d", frame_index);
        ImGui::Text("Scene time: %.1f s", scene.get_time());
    }

    if(ImGui::CollapsingHeader(
         "Rasterizer",
         ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool wireframe = viewport.draw_params.wireframe;
        bool cull_face = viewport.draw_params.cull_face;
        bool paused = scene.is_paused();

        if(ImGui::Checkbox("Paused", &paused))
        {
            scene.set_paused(paused);
        }

        if(ImGui::Checkbox("Wireframe", &wireframe))
        {
            viewport.draw_params.wireframe = wireframe;
        }

        if(ImGui::Checkbox("Face Culling", &cull_face))
        {
            viewport.draw_params.cull_face = cull_face;
        }
    }

    if(ImGui::CollapsingHeader(
         "Stats",
         ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("ms/frame: %.3f", 1000.0f / std::max(io.Framerate, 0.001f));
        ImGui::Text("render time: %.3f ms", 1000.f * renderer.get_render_time());
    }

    ImGui::End();
}

}    // namespace imgui

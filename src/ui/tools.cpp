/**
 * Software Rasterizer Playground.
 *
 * tool panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cstddef>

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

        const char* navigation_modes[] = {
          "FPS",
          "Orbit",
        };
        int navigation_mode = static_cast<int>(viewport.get_navigation_mode());
        if(ImGui::Combo(
             "RMB Mode",
             &navigation_mode,
             navigation_modes,
             IM_ARRAYSIZE(navigation_modes)))
        {
            viewport.set_navigation_mode(
              static_cast<ViewportNavigationMode>(navigation_mode));
        }
    }

    if(ImGui::CollapsingHeader(
         "Rasterizer",
         ImGuiTreeNodeFlags_DefaultOpen))
    {
        ViewportDisplaySettings display_settings = viewport.get_display_settings();
        ViewportOverlaySettings overlay_settings = viewport.get_overlay_settings();

        bool paused = scene.is_paused();

        if(ImGui::Checkbox("Paused", &paused))
        {
            scene.set_paused(paused);
        }

        bool update_overlay_settings = false;

        if(ImGui::Checkbox("Grid", &overlay_settings.show_grid))
        {
            update_overlay_settings = true;
        }

        bool update_display_settings = false;

        if(ImGui::Checkbox("Wireframe", &display_settings.wireframe))
        {
            update_display_settings = true;
        }

        if(ImGui::Checkbox("Face Culling", &display_settings.cull_face))
        {
            update_display_settings = true;
        }

        if(ImGui::Checkbox("Frustum Culling", &display_settings.cull_frustum))
        {
            update_display_settings = true;
        }

        if(ImGui::Checkbox("Dynamic LOD", &display_settings.dynamic_lod))
        {
            update_display_settings = true;
        }

        if(update_overlay_settings)
        {
            viewport.set_overlay_settings(overlay_settings);
        }

        if(update_display_settings)
        {
            viewport.set_display_settings(display_settings);
        }
    }

    if(ImGui::CollapsingHeader(
         "Stats",
         ImGuiTreeNodeFlags_DefaultOpen))
    {
        const RendererStats& stats = renderer.get_stats();

        if(ImGui::BeginTable(
             "FrameStats",
             2,
             ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("FPS");
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", io.Framerate);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Frame");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f ms", 1000.0f / std::max(io.Framerate, 0.001f));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Render");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f ms", 1000.f * renderer.get_render_time());

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("Meshes");

        if(ImGui::BeginTable(
             "MeshStats",
             2,
             ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Static meshes");
            ImGui::TableNextColumn();
            ImGui::Text(
              "%llu",
              static_cast<unsigned long long>(stats.static_meshes));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Sections drawn");
            ImGui::TableNextColumn();
            ImGui::Text(
              "%llu",
              static_cast<unsigned long long>(stats.mesh_sections_drawn));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Sections culled");
            ImGui::TableNextColumn();
            ImGui::Text(
              "%llu",
              static_cast<unsigned long long>(stats.mesh_sections_culled));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Triangles submitted");
            ImGui::TableNextColumn();
            ImGui::Text(
              "%llu",
              static_cast<unsigned long long>(stats.triangles_submitted));

            for(std::size_t lod_index = 0;
                lod_index < stats.static_mesh_lods_selected.size();
                ++lod_index)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("LOD %llu selected", static_cast<unsigned long long>(lod_index));
                ImGui::TableNextColumn();
                ImGui::Text(
                  "%llu",
                  static_cast<unsigned long long>(
                    stats.static_mesh_lods_selected[lod_index]));
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}

}    // namespace imgui

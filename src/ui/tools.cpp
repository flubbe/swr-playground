/**
 * Software Rasterizer Playground.
 *
 * Tools panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cstddef>
#include <format>

#include <imgui.h>

#include "containers/format.h"
#include "scene/directionallight.h"
#include "scene/scene.h"
#include "renderer/renderdevice.h"
#include "renderer/renderer.h"
#include "viewport.h"
#include "application.h"

namespace imgui
{

static bool sorting_benchmark_requested = false;
static int sorting_benchmark_iterations = 100;
static int sorting_depth_bin_count = 8;

void draw_tools_panel(
  Application& app,
  RenderDevice& render_device,
  Viewport& viewport,
  Scene& scene,
  Renderer& renderer,
  int frame_index,
  float pixel_density)
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

        const char* shader_names[] = {
          "Flat",
          "Smooth",
          "Phong",
          "Shadowed"};
        int shader_index = static_cast<int>(app.get_static_mesh_shader());
        if(ImGui::Combo("Shader", &shader_index, shader_names, IM_ARRAYSIZE(shader_names)))
        {
            app.set_static_mesh_shader(static_cast<StaticMeshShaderType>(shader_index));
        }

        const char* floor_shader_names[] = {
          "Textured",
          "Textured Shiny"};
        int floor_shader_index = static_cast<int>(app.get_floor_shader());
        if(ImGui::Combo(
             "Floor Shader",
             &floor_shader_index,
             floor_shader_names,
             IM_ARRAYSIZE(floor_shader_names)))
        {
            app.set_floor_shader(static_cast<FloorShaderType>(floor_shader_index));
        }

        const char* light_mode_names[] = {
          "Rotating",
          "Stationary",
        };

        scene.for_each_object<DirectionalLight>(
          [&light_mode_names](DirectionalLight& light, std::size_t light_index)
          {
              int light_mode_index = static_cast<int>(light.behavior);

              const swr::string label = swr::format(
                "Directional Light {}",
                light_index + 1);
              if(ImGui::Combo(
                   label.c_str(),
                   &light_mode_index,
                   light_mode_names,
                   IM_ARRAYSIZE(light_mode_names)))
              {
                  light.behavior = static_cast<DirectionalLightBehavior>(light_mode_index);
              }
          });

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

        if(ImGui::DragFloat(
             "LOD Pixels Per Triangle",
             &display_settings.target_pixels_per_triangle,
             1.f,
             0.5f,
             256.f))
        {
            update_display_settings = true;
        }

        if(ImGui::Checkbox("Sort Meshes", &display_settings.sort_meshes))
        {
            update_display_settings = true;
        }

        if(display_settings.sort_meshes)
        {
            const SortMode current_mode = renderer.get_sort_mode();
            const std::string_view current_name = RenderQueueSortFactory::get_name(current_mode);

            if(ImGui::BeginCombo("Sort Mode", current_name.data()))
            {
                for(SortMode mode: RenderQueueSortFactory::supported_modes)
                {
                    const bool is_selected = (mode == current_mode);
                    const char* name = RenderQueueSortFactory::get_name(mode);

                    if(ImGui::Selectable(name, is_selected))
                    {
                        renderer.set_sort_mode(mode);
                    }

                    if(is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        {
            const char* pcf_modes[] = {
              "Off",
              "Legacy 3x3 Nearest",
              "Legacy Bilinear",
              "Modern 3x3 Nearest",
              "Modern 3x3 Bilinear",
              "Stochastic 4-Tap",
              "Stochastic 5-Tap",
              "Stochastic 4-Tap Stable",
              "Stochastic 4-Tap Interleaved"};
            int pcf_mode_int = static_cast<int>(renderer.get_shadow_pcf_mode());
            if(ImGui::Combo("Shadow PCF Mode", &pcf_mode_int, pcf_modes, IM_ARRAYSIZE(pcf_modes)))
            {
                renderer.set_shadow_pcf_mode(static_cast<ShadowPcfMode>(pcf_mode_int));
            }
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

        ImGui::Spacing();
        ImGui::TextUnformatted("Shaders");

        if(ImGui::BeginTable(
             "ShaderStats",
             2,
             ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Cache Size");
            ImGui::TableNextColumn();
            ImGui::Text(
              "%llu",
              static_cast<unsigned long long>(renderer.get_shader_cache().size()));

            ImGui::EndTable();
        }
    }

    if(ImGui::CollapsingHeader("Registered Shaders", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& shader_cache = renderer.get_shader_cache();
        auto names = shader_cache.get_names();

        // Helper struct to parse categories cleanly
        struct ParsedShader
        {
            std::string_view category;
            std::string_view local_name;
            std::string_view full_name;
            std::size_t size{0};
        };

        static swr::vector<ParsedShader> parsed_shaders;
        parsed_shaders.clear();
        parsed_shaders.reserve(names.size());

        for(const auto& name: names)
        {
            const auto* shader = shader_cache.get(name);

            size_t sep_pos = name.find_first_of("./");
            if(sep_pos != std::string::npos)
            {
                parsed_shaders.push_back(
                  {.category = std::string_view(name).substr(0, sep_pos),
                   .local_name = std::string_view(name).substr(sep_pos + 1),
                   .full_name = name,
                   .size = shader->size()});
            }
            else
            {
                parsed_shaders.push_back(
                  {.category = "General",
                   .local_name = name,
                   .full_name = name,
                   .size = shader->size()});
            }
        }

        // Sort primarily by Category, secondarily by Local Name
        std::sort(parsed_shaders.begin(), parsed_shaders.end(),
                  [](const ParsedShader& a, const ParsedShader& b)
                  {
                      if(a.category != b.category)
                          return a.category < b.category;
                      return a.local_name < b.local_name;
                  });

        // Render Tree Nodes
        std::string_view current_category = "";
        bool category_open = false;

        for(const auto& shader: parsed_shaders)
        {
            // When encountering a new category
            if(shader.category != current_category)
            {
                // Pop previous category tree node if it was open
                if(!current_category.empty() && category_open)
                {
                    ImGui::TreePop();
                }

                current_category = shader.category;

                // Use category string as a unique ID scope
                ImGui::PushID(
                  current_category.data(),
                  current_category.data() + current_category.size());
                category_open = ImGui::TreeNodeEx(
                  "CategoryNode",
                  ImGuiTreeNodeFlags_DefaultOpen,
                  "%.*s",
                  static_cast<int>(current_category.size()),
                  current_category.data());
                ImGui::PopID();
            }

            // Render leaf item if the parent node is currently expanded
            if(category_open)
            {
                ImGui::BulletText(
                  "%.*s [%d b]",
                  static_cast<int>(shader.local_name.size()),
                  shader.local_name.data(),
                  static_cast<int>(shader.size));
            }
        }

        // Pop final category node if left open
        if(!current_category.empty() && category_open)
        {
            ImGui::TreePop();
        }
    }

    if(ImGui::CollapsingHeader("Sorting Benchmark", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool benchmark_in_progress = renderer.is_benchmark_in_progress();

        if(benchmark_in_progress)
        {
            ImGui::Text("Benchmark in progress...");
        }
        else
        {
            ImGui::InputInt("Iterations", &sorting_benchmark_iterations);
            sorting_benchmark_iterations = app.set_benchmark_iterations(sorting_benchmark_iterations);

            ImGui::InputInt("Depth Bins", &sorting_depth_bin_count, 1, 4);
            if(sorting_depth_bin_count < 1)
            {
                sorting_depth_bin_count = 1;
            }
            if(static_cast<std::size_t>(sorting_depth_bin_count) != renderer.get_depth_bin_count())
            {
                renderer.set_depth_bin_count(static_cast<std::size_t>(sorting_depth_bin_count));
            }

            if(ImGui::Button("Run Benchmark", ImVec2{-1.f, 0.f}))
            {
                sorting_benchmark_requested = true;
            }

            if(ImGui::Button("Run Comparative Benchmark", ImVec2{-1.f, 0.f}))
            {
                // FIXME Benchmark should not be controlled by the renderer.
                renderer.start_comparative_benchmark(
                  scene,
                  viewport,
                  static_cast<std::size_t>(sorting_benchmark_iterations));
            }

            // FIXME Benchmark should not be controlled by the renderer.
            const SortingBenchmarkResults& results = renderer.get_benchmark_results();
            if(results.iterations > 0)
            {
                ImGui::Spacing();
                ImGui::TextUnformatted("Results:");

                if(ImGui::BeginTable(
                     "BenchmarkResults",
                     2,
                     ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
                {
                    ImGui::TableSetupColumn("Metric");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableHeadersRow();

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Iterations");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", static_cast<unsigned long long>(results.iterations));

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("With Sorting");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f ms", 1000.f * results.time_with_sorting / results.iterations);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Without Sorting");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f ms", 1000.f * results.time_without_sorting / results.iterations);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Difference");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f ms", 1000.f * results.get_difference() / results.iterations);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Improvement");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.1f %%", results.get_percentage_improvement());

                    ImGui::EndTable();
                }

                ImGui::Spacing();
                const auto& comp = renderer.get_comparative_results();
                if(!comp.empty())
                {
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Comparative Results (FullSort / BinSort):");
                    if(ImGui::BeginTable("ComparativeResults", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Mode");
                        ImGui::TableSetupColumn("With Sorting (ms)");
                        ImGui::TableSetupColumn("Without Sorting (ms)");
                        ImGui::TableHeadersRow();

                        const char* labels[] = {"FullSort", "BinSort"};
                        for(size_t i = 0; i < comp.size() && i < 2; ++i)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(labels[i]);
                            ImGui::TableNextColumn();
                            ImGui::Text("%.3f", 1000.f * comp[i].time_with_sorting / std::max<std::size_t>(1, comp[i].iterations));
                            ImGui::TableNextColumn();
                            ImGui::Text("%.3f", 1000.f * comp[i].time_without_sorting / std::max<std::size_t>(1, comp[i].iterations));
                        }
                        ImGui::EndTable();
                    }
                }
                if(results.get_percentage_improvement() > 0.1f)
                {
                    ImGui::TextColored(
                      ImVec4(0.2f, 0.8f, 0.2f, 1.0f),
                      "Sorting improves performance by ~%.1f%%",
                      results.get_percentage_improvement());
                }
                else if(results.get_percentage_improvement() < -0.1f)
                {
                    ImGui::TextColored(
                      ImVec4(0.8f, 0.3f, 0.2f, 1.0f),
                      "Sorting reduces performance by ~%.1f%%",
                      -results.get_percentage_improvement());
                }
                else
                {
                    ImGui::TextColored(
                      ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                      "No significant difference (%.2f%%)",
                      results.get_percentage_improvement());
                }
            }
        }
    }

    ImGui::End();
}

bool check_and_clear_sorting_benchmark_request()
{
    bool result = sorting_benchmark_requested;
    sorting_benchmark_requested = false;
    return result;
}

}    // namespace imgui

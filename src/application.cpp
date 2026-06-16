/**
 * Software Rasterizer Playground.
 *
 * Main application.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "assets/static_mesh_importer.h"
#include "assets/texture.h"
#include "scene/gear.h"
#include "scene/scene.h"
#include "scene/static_mesh.h"
#include "ui/imgui.h"
#include "application.h"
#include "logging.h"
#include "mesh_lod.h"
#include "renderdevice.h"
#include "renderer.h"
#include "shader_cache.h"
#include "viewport.h"

namespace
{

class SDLError : public std::runtime_error
{
public:
    explicit SDLError(
      std::string_view message)
    : std::runtime_error{
        std::format(
          "{}: {}",
          message,
          SDL_GetError())}
    {
    }
};

ViewportEditorCameraInput gather_viewport_camera_input(
  const ViewportInputState& viewport_input,
  const ImGuiIO& io,
  bool mouse_captured,
  ViewportNavigationMode mode)
{
    ViewportEditorCameraInput input{};

    if(!mouse_captured)
    {
        return input;
    }

    input.active = true;
    input.look_yaw = viewport_input.mouse_delta_x;
    input.look_pitch = viewport_input.mouse_delta_y;
    if(viewport_input.viewport_hovered)
    {
        input.zoom_delta = viewport_input.mouse_wheel_delta;
    }

    const bool keyboard_blocked = io.WantCaptureKeyboard;
    if(keyboard_blocked)
    {
        return input;
    }

    if(mode == ViewportNavigationMode::Orbit)
    {
        return input;
    }

    const bool* keys = SDL_GetKeyboardState(nullptr);
    if(keys == nullptr)
    {
        return input;
    }

    if(keys[SDL_SCANCODE_W])
    {
        input.move_forward += 1.f;
    }
    if(keys[SDL_SCANCODE_S])
    {
        input.move_forward -= 1.f;
    }
    if(keys[SDL_SCANCODE_D])
    {
        input.move_right += 1.f;
    }
    if(keys[SDL_SCANCODE_A])
    {
        input.move_right -= 1.f;
    }
    if(keys[SDL_SCANCODE_E])
    {
        input.move_up += 1.f;
    }
    if(keys[SDL_SCANCODE_Q])
    {
        input.move_up -= 1.f;
    }

    input.fast_move = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    return input;
}

GLuint create_viewport_texture(
  int width,
  int height)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    if(texture == 0)
    {
        throw std::runtime_error{"glGenTextures failed"};
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_SRGB8_ALPHA8,
      width,
      height,
      0,
      GL_BGRA,
      GL_UNSIGNED_INT_8_8_8_8_REV,
      nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void destroy_viewport_texture(GLuint& texture)
{
    if(texture != 0)
    {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}

void update_viewport_texture(
  GLuint texture,
  const RenderDevice& render_device)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
      GL_TEXTURE_2D,
      0,
      0,
      0,
      render_device.get_width(),
      render_device.get_height(),
      GL_BGRA,
      GL_UNSIGNED_INT_8_8_8_8_REV,
      render_device.get_data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void set_imgui_mouse_interactions_enabled(bool enabled)
{
    ImGuiIO& io = ImGui::GetIO();
    if(enabled)
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }
    else
    {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
}

bool viewport_contains_mouse_position(
  const ViewportInputState& viewport_input,
  float x,
  float y)
{
    return viewport_input.viewport_rect_valid
           && x >= viewport_input.viewport_min_x
           && x < viewport_input.viewport_max_x
           && y >= viewport_input.viewport_min_y
           && y < viewport_input.viewport_max_y;
}

void imgui_draw_viewport_panel(
  RenderDevice& render_device,
  Renderer& renderer,
  Scene& scene,
  Viewport& viewport,
  GLuint& viewport_texture,
  bool& running,
  ViewportInputState& viewport_input)
{
    ImGui::Begin("Viewport");

    ImVec2 avail = ImGui::GetContentRegionAvail();

    // ImGui sizes are logical units; rasterizer target should use pixels.
    int viewport_w_px = std::max(1, static_cast<int>(std::round(avail.x)));
    int viewport_h_px = std::max(1, static_cast<int>(std::round(avail.y)));

    // FIXME the dimensions should not come from the render device
    if(viewport_w_px != render_device.get_width()
       || viewport_h_px != render_device.get_height())
    {
        viewport.set_resolution(viewport_w_px, viewport_h_px);
        render_device.resize(viewport_w_px, viewport_h_px);

        destroy_viewport_texture(viewport_texture);

        try
        {
            viewport_texture = create_viewport_texture(
              render_device.get_width(),
              render_device.get_height());
            logging::logf(
              "resized viewport to {}x{}",
              render_device.get_width(),
              render_device.get_height());
        }
        catch(const std::exception& e)
        {
            logging::errorf("{}", e.what());
            running = false;
        }
    }

    viewport.update_active_camera_projection(scene);

    if(renderer.is_benchmark_in_progress())
    {
        renderer.update_sorting_benchmark(scene, viewport);
    }
    else
    {
        renderer.render(
          scene,
          viewport);
    }

    if(viewport_texture != 0)
    {
        update_viewport_texture(viewport_texture, render_device);

        // Display at logical UI size, not pixel size.
        ImGui::Image(
          static_cast<ImTextureID>(viewport_texture),
          avail,
          ImVec2{0, 0},
          ImVec2{1, 1});
        const ImVec2 viewport_min = ImGui::GetItemRectMin();
        const ImVec2 viewport_max = ImGui::GetItemRectMax();
        viewport_input.viewport_hovered = ImGui::IsItemHovered();
        viewport_input.viewport_rect_valid = true;
        viewport_input.viewport_min_x = viewport_min.x;
        viewport_input.viewport_min_y = viewport_min.y;
        viewport_input.viewport_max_x = viewport_max.x;
        viewport_input.viewport_max_y = viewport_max.y;

        if(renderer.is_benchmark_in_progress())
        {
            const std::size_t iteration =
              renderer.get_benchmark_current_iteration();
            const std::size_t target =
              renderer.get_benchmark_target_iterations();
            const char* phase = renderer.is_benchmark_sorted_phase()
                                  ? "With Sorting"
                                  : "Without Sorting";
            const std::string status = std::format(
              "Benchmark running: {} {}/{}",
              phase,
              iteration + 1,
              target);
            const ImVec2 text_pos = ImVec2{
              viewport_min.x + 8.0f,
              viewport_min.y + 28.0f};
            const ImVec2 text_size = ImGui::CalcTextSize(status.c_str());
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(
              ImVec2{text_pos.x - 4.f, text_pos.y - 4.f},
              ImVec2{text_pos.x + text_size.x + 4.f,
                     text_pos.y + text_size.y + 4.f},
              IM_COL32(0, 0, 0, 180),
              4.0f);
            draw_list->AddText(
              text_pos,
              IM_COL32(255, 255, 255, 255),
              status.c_str());
        }

        if(viewport.is_camera_selector_overlay_enabled())
        {
            const ViewportCameraType camera_type = viewport.get_camera_type(scene);
            std::string camera_name{to_string(viewport.get_editor_camera_view())};
            if(camera_type == ViewportCameraType::Scene)
            {
                camera_name = viewport.get_camera(scene).get_name();
            }

            const std::string label_left = "[";
            const std::string label_name = camera_name;
            const std::string label_right = "]";
            const std::string label = label_left + label_name + label_right;
            const ImVec2 text_pos = ImVec2{
              viewport_min.x + 8.0f,
              viewport_min.y + 6.0f};

            // Context-menu trigger area over camera label (right-click like DCC/CAD tools).
            const ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
            ImGui::SetCursorScreenPos(text_pos);
            ImGui::InvisibleButton("viewport_camera_overlay_menu_trigger", label_size);
            const bool is_hovered = ImGui::IsItemHovered();
            const bool is_menu_open = ImGui::IsPopupOpen("viewport_camera_overlay_menu");
            const bool is_active = is_hovered || is_menu_open;
            if(is_hovered)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const ImU32 bracket_color = IM_COL32(235, 235, 235, 255);
            const ImU32 name_color = is_active
                                       ? IM_COL32(255, 224, 120, 255)
                                       : bracket_color;
            const ImVec2 left_size = ImGui::CalcTextSize(label_left.c_str());
            const ImVec2 name_size = ImGui::CalcTextSize(label_name.c_str());

            draw_list->AddText(
              text_pos,
              bracket_color,
              label_left.c_str());
            draw_list->AddText(
              ImVec2{text_pos.x + left_size.x, text_pos.y},
              name_color,
              label_name.c_str());
            draw_list->AddText(
              ImVec2{text_pos.x + left_size.x + name_size.x, text_pos.y},
              bracket_color,
              label_right.c_str());
            if(ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                ImGui::OpenPopup("viewport_camera_overlay_menu");
            }
            if(ImGui::BeginPopup("viewport_camera_overlay_menu"))
            {
                const bool using_local_camera =
                  (viewport.get_camera_type(scene) == ViewportCameraType::Local);
                for(int view_index = 0;
                    view_index <= static_cast<int>(EditorCameraView::Orthographic);
                    ++view_index)
                {
                    const auto view = static_cast<EditorCameraView>(view_index);
                    const bool selected =
                      using_local_camera && viewport.get_editor_camera_view() == view;
                    if(ImGui::MenuItem(
                         to_string(view).data(),
                         nullptr,
                         selected))
                    {
                        viewport.use_local_camera();
                        viewport.set_editor_camera_view(view);
                    }
                }

                const std::optional<ObjectId> selected_scene_camera_id =
                  viewport.get_scene_camera_id();
                const std::vector<Camera*> scene_cameras = scene.get_cameras();

                if(ImGui::BeginMenu("Cameras"))
                {
                    bool has_any_scene_camera = false;
                    for(const Camera* camera: scene_cameras)
                    {
                        if(camera == nullptr)
                        {
                            continue;
                        }
                        has_any_scene_camera = true;
                        const bool selected =
                          selected_scene_camera_id.has_value()
                          && selected_scene_camera_id.value() == camera->get_object_id()
                          && viewport.get_camera_type(scene) == ViewportCameraType::Scene;
                        if(ImGui::MenuItem(
                             camera->get_name().c_str(),
                             nullptr,
                             selected))
                        {
                            viewport.use_scene_camera(camera->get_object_id());
                        }
                    }

                    if(!has_any_scene_camera)
                    {
                        ImGui::BeginDisabled(true);
                        ImGui::MenuItem("<No Scene Cameras>");
                        ImGui::EndDisabled();
                    }

                    ImGui::EndMenu();
                }

                const bool using_scene_camera =
                  viewport.get_camera_type(scene) == ViewportCameraType::Scene;
                if(using_scene_camera)
                {
                    ImGui::BeginDisabled();
                }
                if(ImGui::MenuItem("Reset") && !using_scene_camera)
                {
                    viewport.reset_editor_camera();
                }
                if(using_scene_camera)
                {
                    ImGui::EndDisabled();
                }

                ImGui::EndPopup();
            }
        }
    }
    else
    {
        viewport_input.viewport_hovered = false;
        viewport_input.viewport_rect_valid = false;
    }

    ImGui::End();
}

/*
 * Debug scene creation.
 */

struct GearBuildParams
{
    ml::vec4 color;
    float inner_radius;
    float outer_radius;
    float width;
    int teeth;
    float tooth_depth;
};

void expand_mesh_handle_bounds(
  MeshBounds& bounds,
  const RenderDevice& device,
  std::uint32_t mesh_handle)
{
    const MeshBounds* mesh_bounds = device.get_mesh_bounds(mesh_handle);
    if(mesh_bounds != nullptr)
    {
        expand_bounds(bounds, *mesh_bounds);
    }
}

MeshBounds calculate_mesh_section_bounds(
  const RenderDevice& device,
  const std::vector<MeshSection>& sections)
{
    MeshBounds bounds;
    for(const MeshSection& section: sections)
    {
        expand_mesh_handle_bounds(
          bounds,
          device,
          section.mesh_handle);
    }

    return bounds;
}

GearParameters create_gear_resources(
  RenderDevice& device,
  ShaderCache& shader_cache,
  const GearBuildParams& p)
{
    auto* flat_shader = shader_cache.get_or_create<shader::ColorFlat>();
    auto* smooth_shader = shader_cache.get_or_create<shader::ColorSmooth>();

    auto flat_material = device.create_material(*flat_shader);
    auto smooth_material = device.create_material(*smooth_shader);

    auto geom = make_gear(
      p.inner_radius,
      p.outer_radius,
      p.width,
      p.teeth,
      p.tooth_depth);

    auto inner_mesh = device.create_mesh(
      MeshData{
        .primitive_type = PrimitiveType::Triangles,
        .indices = std::move(geom.inner_indices),
        .vertices = std::move(geom.inner_vertices),
        .normals = std::move(geom.inner_normals),
      });

    auto outer_mesh = device.create_mesh(
      MeshData{
        .primitive_type = PrimitiveType::Triangles,
        .indices = std::move(geom.outer_indices),
        .vertices = std::move(geom.outer_vertices),
        .normals = std::move(geom.outer_normals),
      });

    MeshBounds bounds;
    expand_mesh_handle_bounds(
      bounds,
      device,
      inner_mesh);
    expand_mesh_handle_bounds(
      bounds,
      device,
      outer_mesh);

    return GearParameters{
      .inner = MeshSection{
        .mesh_handle = inner_mesh,
        .material_handle = smooth_material,
        .color = p.color,
      },
      .outer = MeshSection{
        .mesh_handle = outer_mesh,
        .material_handle = flat_material,
        .color = p.color,
      },
      .bounds = bounds,
      .inner_radius = p.inner_radius,
      .outer_radius = p.outer_radius,
      .width = p.width,
      .teeth = p.teeth,
      .tooth_depth = p.tooth_depth,
    };
}

class GearFactory
{
    RenderDevice& device;
    ShaderCache& shader_cache;

public:
    GearFactory(
      RenderDevice& device,
      ShaderCache& shader_cache)
    : device{device}
    , shader_cache{shader_cache}
    {
    }

    Gear& create(
      Scene& scene,
      const GearBuildParams& build,
      const ml::mat4x4& transform)
    {
        auto params = create_gear_resources(device, shader_cache, build);
        auto* gear = scene.add_object<Gear>(params);
        gear->set_transform(transform);
        return *gear;
    }
};

MeshBounds calculate_imported_mesh_bounds(
  const ImportedStaticMesh& imported_mesh)
{
    MeshBounds bounds;
    for(const auto& mesh: imported_mesh.meshes)
    {
        expand_bounds(
          bounds,
          calculate_mesh_bounds(mesh.mesh_data));
    }

    return bounds;
}

float calculate_max_half_extent(
  const MeshBounds& bounds)
{
    if(!bounds.valid)
    {
        return 0.f;
    }

    const ml::vec3 extents = bounds.max - bounds.min;
    return 0.5f * std::max({extents.x, extents.y, extents.z});
}

ml::vec3 calculate_center(
  const MeshBounds& bounds)
{
    return (bounds.min + bounds.max) * 0.5f;
}

ml::mat4x4 make_static_mesh_fit_transform(
  const MeshBounds& bounds,
  float target_half_extent)
{
    const float max_half_extent = calculate_max_half_extent(bounds);
    if(max_half_extent <= std::numeric_limits<float>::epsilon())
    {
        return ml::mat4x4::identity();
    }

    const ml::vec3 center = calculate_center(bounds);
    const float scale = target_half_extent / max_half_extent;

    ml::mat4x4 fit_transform =
      ml::matrices::scaling(scale)
      * ml::matrices::translation(-center);

    return fit_transform;
}

struct StaticMeshAssetResources
{
    std::vector<StaticMeshLod> lods;
    ml::mat4x4 fit_transform;
    std::string name;
};

constexpr std::string_view floor_object_name = "Stone Floor";

std::vector<StaticMeshLod> create_static_mesh_resources(
  RenderDevice& device,
  ShaderCache& shader_cache,
  ImportedStaticMesh imported_mesh)
{
    const StaticMeshLodBuildSettings lod_settings{
      .preserve_boundaries = false,
      .recompute_normals = true,
    };

    std::vector<StaticMeshLod> result_lods;
    result_lods.resize(lod_settings.lods.size());

    for(std::size_t i = 0; i < lod_settings.lods.size(); ++i)
    {
        result_lods[i].min_screen_height = lod_settings.lods[i].min_screen_height;
    }

    StaticMeshLodBuilder lod_builder;

    for(auto& mesh: imported_mesh.meshes)
    {
        auto* shader = shader_cache.get_or_create<shader::PhongSmooth>();

        const std::uint32_t material = device.create_material(*shader);

        const StaticMeshLodBuildResult lod_build_result =
          lod_builder.build(
            mesh.mesh_data,
            lod_settings);

        for(std::size_t lod_index = 0;
            lod_index < lod_build_result.lod_meshes.size() && lod_index < result_lods.size();
            ++lod_index)
        {
            const auto& stats = lod_build_result.simplify_stats[lod_index];
            auto& lod_mesh = lod_build_result.lod_meshes[lod_index];

            logging::logf(
              "LOD frac {}, indices {}, tris {}, accepted {}, rejected {}, queued {}, boundary {}",
              lod_settings.lods[lod_index].triangle_fraction,
              lod_mesh.mesh.indices.size(),
              lod_mesh.mesh.indices.size() / 3,
              stats.accepted_collapses,
              stats.rejected_collapses,
              stats.queued_edges,
              stats.boundary_vertices);

            const std::uint32_t mesh_handle =
              device.create_mesh(std::move(lod_mesh.mesh));
            expand_mesh_handle_bounds(
              result_lods[lod_index].bounds,
              device,
              mesh_handle);

            result_lods[lod_index].mesh_sections.push_back(
              MeshSection{
                .mesh_handle = mesh_handle,
                .material_handle = material,
                .color = mesh.diffuse_color,
              });
        }
    }

    std::erase_if(
      result_lods,
      [](const StaticMeshLod& lod)
      {
          return lod.mesh_sections.empty();
      });

    return result_lods;
}

std::optional<StaticMeshAssetResources> try_create_static_mesh_resources_from_file(
  RenderDevice& device,
  ShaderCache& shader_cache,
  const std::filesystem::path& path)
{
    if(!std::filesystem::exists(path))
    {
        logging::logf(
          "static mesh asset not found: {}",
          path.string());
        return std::nullopt;
    }

    try
    {
        constexpr float preview_half_extent = 2.f;

        ImportedStaticMesh imported_mesh = import_static_mesh(path);
        const MeshBounds mesh_bounds =
          calculate_imported_mesh_bounds(imported_mesh);
        const ml::mat4x4 fit_transform = make_static_mesh_fit_transform(
          mesh_bounds,
          preview_half_extent);
        auto lods = create_static_mesh_resources(
          device,
          shader_cache,
          std::move(imported_mesh));

        if(lods.empty())
        {
            logging::warningf(
              "static mesh asset has no renderable meshes: {}",
              path.string());
            return std::nullopt;
        }

        logging::logf(
          "imported static mesh: {}",
          path.string());

        return StaticMeshAssetResources{
          .lods = std::move(lods),
          .fit_transform = fit_transform,
          .name = path.filename().string(),
        };
    }
    catch(const std::exception& e)
    {
        logging::warningf(
          "failed to import static mesh '{}': {}",
          path.string(),
          e.what());
    }

    return std::nullopt;
}

MeshData make_floor_mesh(
  float half_extent,
  float uv_repeat)
{
    const ml::vec4 up_normal{0.f, 1.f, 0.f, 0.f};

    return MeshData{
      .primitive_type = PrimitiveType::Triangles,
      .indices = {0, 2, 1, 0, 3, 2},
      .vertices = {
        {-half_extent, 0.f, -half_extent, 1.f},
        {half_extent, 0.f, -half_extent, 1.f},
        {half_extent, 0.f, half_extent, 1.f},
        {-half_extent, 0.f, half_extent, 1.f},
      },
      .normals = {
        up_normal,
        up_normal,
        up_normal,
        up_normal,
      },
      .texcoords = {
        {0.f, uv_repeat, 0.f, 0.f},
        {uv_repeat, uv_repeat, 0.f, 0.f},
        {uv_repeat, 0.f, 0.f, 0.f},
        {0.f, 0.f, 0.f, 0.f},
      },
    };
}

void try_add_textured_floor(
  Scene& scene,
  RenderDevice& device,
  ShaderCache& shader_cache)
{
    const std::filesystem::path diffuse_path{
      "assets/textures/tiles/tiles_0080_color_1k.png"};
    const std::filesystem::path normal_path{
      "assets/textures/tiles/tiles_0080_normal_opengl_1k.png"};

    if(!std::filesystem::exists(diffuse_path)
       || !std::filesystem::exists(normal_path))
    {
        logging::warningf(
          "floor textures not found: '{}' or '{}'",
          diffuse_path.string(),
          normal_path.string());
        return;
    }

    std::optional<std::uint32_t> diffuse_texture;
    std::optional<std::uint32_t> normal_texture;
    std::optional<std::uint32_t> mesh_handle;

    try
    {
        constexpr float floor_half_extent = 28.f;
        constexpr float uv_repeat = 1.f;
        constexpr float floor_height = -6.25f;

        diffuse_texture = device.create_texture(
          assets::load_texture_rgba8(diffuse_path));
        normal_texture = device.create_texture(
          assets::load_normal_map_rgba8(
            normal_path,
            assets::NormalMapConvention::DirectX));
        auto* shader = shader_cache.get_or_create<shader::TexturedFloor>();

        mesh_handle = device.create_mesh(
          make_floor_mesh(
            floor_half_extent,
            uv_repeat));
        const std::array<std::uint32_t, 2> textures = {
          *diffuse_texture,
          *normal_texture};
        const std::uint32_t material_handle = device.create_material(
          *shader,
          textures);
        diffuse_texture.reset();
        normal_texture.reset();
        const MeshBounds bounds = *device.get_mesh_bounds(*mesh_handle);

        auto* floor = scene.add_object<StaticMesh>(
          std::vector<MeshSection>{
            MeshSection{
              .mesh_handle = *mesh_handle,
              .material_handle = material_handle,
              .color = {1.f, 1.f, 1.f, 1.f},
            }},
          bounds);
        floor->set_name(std::string{floor_object_name});
        floor->set_transform(ml::matrices::translation(0.f, floor_height, 0.f));
        floor->capture_snapshot();
    }
    catch(const std::exception& e)
    {
        if(mesh_handle.has_value())
        {
            device.delete_mesh(*mesh_handle);
        }
        if(diffuse_texture.has_value())
        {
            device.delete_texture(*diffuse_texture);
        }
        if(normal_texture.has_value())
        {
            device.delete_texture(*normal_texture);
        }
        logging::warningf(
          "failed to create textured floor: {}",
          e.what());
    }
}

StaticMesh* create_static_mesh_instance(
  Scene& scene,
  const StaticMeshAssetResources& resources,
  const ml::mat4x4& transform)
{
    StaticMesh* mesh = scene.add_object<StaticMesh>(
      resources.lods);
    mesh->set_name(resources.name);
    mesh->set_transform(transform * resources.fit_transform);
    mesh->capture_snapshot();
    return mesh;
}

void rebuild_gear_mesh_if_needed(
  RenderDevice& device,
  Gear* gear)
{
    if(gear == nullptr)
    {
        return;
    }

    gear->clamp_runtime_parameters();
    if(!gear->needs_rebuild())
    {
        return;
    }

    const int teeth = std::clamp(
      gear->get_teeth(),
      gear_limits::min_teeth,
      gear_limits::max_teeth);

    const auto old_mesh_sections = gear->get_mesh_sections();
    if(old_mesh_sections.size() != 2)
    {
        return;
    }

    auto geom = make_gear(
      gear->get_inner_radius(),
      gear->get_outer_radius(),
      gear->get_width(),
      teeth,
      gear->get_tooth_depth());

    const std::uint32_t old_inner_mesh = old_mesh_sections[0].mesh_handle;
    const std::uint32_t old_outer_mesh = old_mesh_sections[1].mesh_handle;

    const bool inner_updated = device.update_mesh(
      old_inner_mesh,
      MeshData{
        .primitive_type = PrimitiveType::Triangles,
        .indices = std::move(geom.inner_indices),
        .vertices = std::move(geom.inner_vertices),
        .normals = std::move(geom.inner_normals),
      });
    const bool outer_updated = device.update_mesh(
      old_outer_mesh,
      MeshData{
        .primitive_type = PrimitiveType::Triangles,
        .indices = std::move(geom.outer_indices),
        .vertices = std::move(geom.outer_vertices),
        .normals = std::move(geom.outer_normals),
      });

    if(!inner_updated || !outer_updated)
    {
        return;
    }

    gear->set_mesh_sections(
      old_mesh_sections,
      calculate_mesh_section_bounds(
        device,
        old_mesh_sections));
    gear->mark_rebuilt();
}

void configure_default_directional_lights(Scene& scene)
{
    auto* key_light = scene.add_object<DirectionalLight>();
    key_light->enabled = false;
    key_light->set_name("Key Light");
    key_light->behavior = DirectionalLightBehavior::Rotating;
    key_light->brightness = 0.55f;
    key_light->set_transform(
      ml::matrices::rotation_y(ml::to_radians(210.f))
      * ml::matrices::rotation_x(ml::to_radians(-35.f)));
    key_light->set_position({5.f, 8.f, 10.f});
    key_light->capture_snapshot();

    auto* fill_light = scene.add_object<DirectionalLight>();
    fill_light->set_name("Fill Light");
    fill_light->behavior = DirectionalLightBehavior::Stationary;
    fill_light->brightness = 0.75f;
    fill_light->set_transform(
      ml::matrices::rotation_y(ml::to_radians(35.f))
      * ml::matrices::rotation_x(ml::to_radians(-55.f)));
    fill_light->set_position({-10.f, 12.f, -6.f});
    fill_light->capture_snapshot();
}

void configure_default_spot_lights(Scene& scene)
{
    auto* spotlight = scene.add_object<SpotLight>();
    spotlight->set_name("Spot Light");
    spotlight->color = {0.42f, 0.62f, 1.f, 1.f};
    spotlight->brightness = 7.f;
    spotlight->inner_cone_angle_radians = ml::to_radians(20.f);
    spotlight->outer_cone_angle_radians = ml::to_radians(21.f);
    spotlight->range = 45.f;

    const ml::vec3 spotlight_position{0.f, 11.f, 12.f};
    const ml::vec3 direction_to_origin =
      (-spotlight_position).normalized();
    const float spotlight_pitch =
      std::asin(direction_to_origin.y);
    const float spotlight_yaw =
      std::atan2(-direction_to_origin.x, -direction_to_origin.z);

    spotlight->set_transform(
      ml::matrices::rotation_y(spotlight_yaw)
      * ml::matrices::rotation_x(spotlight_pitch));
    spotlight->set_position(spotlight_position);
    spotlight->capture_snapshot();
}

}    // namespace

void Application::setup_scene()
{
    configure_default_directional_lights(scene);
    configure_default_spot_lights(scene);

    struct GearInit
    {
        GearBuildParams build;
        ml::mat4x4 transform;
        ml::vec3 translation;
        float angular_speed;
        float phase_offset;
    };

    std::array<GearInit, 3> gears = {{
      {
        .build = {.color = {1, 0, 0, 1}, .inner_radius = 1.0f, .outer_radius = 4.0f, .width = 1.0f, .teeth = 20, .tooth_depth = 0.7f},
        .transform = ml::matrices::translation(-3.f, -2.f, 0.f),
        .translation = {-3.f, -2.f, 0.f},
        .angular_speed = 1.f,
        .phase_offset = 0.f,
      },
      {
        .build = {.color = {0, 1, 0, 1}, .inner_radius = 0.5f, .outer_radius = 2.0f, .width = 2.0f, .teeth = 10, .tooth_depth = 0.7f},
        .transform = ml::matrices::translation(3.1f, -2.f, 0.f),
        .translation = {3.1f, -2.f, 0.f},
        .angular_speed = -2.f,
        .phase_offset = -9.f,
      },
      {
        .build = {.color = {0, 0, 1, 1}, .inner_radius = 1.3f, .outer_radius = 2.0f, .width = 0.5f, .teeth = 10, .tooth_depth = 0.7f},
        .transform = ml::matrices::translation(-3.1f, 4.2f, 0.f),
        .translation = {-3.1f, 4.2f, 0.f},
        .angular_speed = -2.f,
        .phase_offset = -25.f,
      },
    }};

    GearFactory factory{render_device, renderer.get_shader_cache()};
    ShaderCache& shader_cache = renderer.get_shader_cache();

    try_add_textured_floor(
      scene,
      render_device,
      shader_cache);

    for(std::size_t i = 0; i < gears.size(); ++i)
    {
        Gear& gear = factory.create(scene, gears[i].build, gears[i].transform);
        scene.set_spin_animation(
          gear.get_object_id(),
          {.translation = gears[i].translation,
           .angular_speed = gears[i].angular_speed,
           .phase_offset = gears[i].phase_offset});
    }

    // Temporary placeholder until asset path resolution moves behind an asset manager.
    const std::filesystem::path static_mesh_path{
      "assets/models/bunny.obj"};

    const auto mesh_resources = try_create_static_mesh_resources_from_file(
      render_device,
      shader_cache,
      static_mesh_path);

    if(mesh_resources.has_value())
    {
        for(int x = -4; x < 5; ++x)
        {
            for(int y = -4; y < 5; ++y)
            {
                create_static_mesh_instance(
                  scene,
                  *mesh_resources,
                  ml::matrices::translation(x * 5.f, 0.f, y * 5.f));
            }
        }
    }

    viewport.reset_editor_camera();

    Camera* camera = scene.add_object<Camera>();
    camera->set_transform(viewport.get_local_camera().get_transform());
    camera->set_name("Editor Camera");
    camera->capture_snapshot();

    viewport.use_local_camera();
}

void Application::setup_viewport()
{
    // Keep the local camera initialized as a fallback if the bound scene camera is removed.
    viewport.reset_editor_camera();
}

Application::Application(
  std::string_view title,
  logging::BufferedLogDevice& log_device,
  RenderDevice& render_device,
  Renderer& renderer,
  Scene& scene,
  Viewport& viewport)
: title{title}
, log_device{log_device}
, render_device{render_device}
, renderer{renderer}
, scene{scene}
, viewport{viewport}
{
    window = SDL_CreateWindow(
      "SWR Playground",
      1280,
      800,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if(!window)
    {
        throw SDLError{"SDL_CreateWindow failed"};
    }

    gl_context = SDL_GL_CreateContext(window);
    if(!gl_context)
    {
        throw SDLError{"SDL_GL_CreateContext failed"};
    }

    if(!SDL_GL_MakeCurrent(window, gl_context))
    {
        throw SDLError{"SDL_GL_MakeCurrent failed"};
    }

    SDL_GL_SetSwapInterval(1);

    if(!imgui::init(window, gl_context))
    {
        throw std::runtime_error{"imgui::init failed."};
    }

    pixel_density = SDL_GetWindowPixelDensity(window);
    display_scale = SDL_GetWindowDisplayScale(window);

    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);

    logging::logf("display scale: {}", display_scale);
    logging::logf("pixel density: {}", pixel_density);
    logging::logf("window size: {} x {}", window_w, window_h);
    logging::logf("pixel size: {} x {}", pixel_w, pixel_h);

    /*
     * viewport setup.
     */

    viewport_texture = create_viewport_texture(
      render_device.get_width(),
      render_device.get_height());
    viewport.set_resolution(
      render_device.get_width(),
      render_device.get_height());

    /*
     * scene setup.
     */

    setup_scene();
    scene.add_default_systems();
    setup_viewport();
}

Application::~Application()
{
    set_viewport_mouse_capture(false);

    imgui::shutdown();

    destroy_viewport_texture(viewport_texture);

    if(gl_context)
    {
        SDL_GL_DestroyContext(gl_context);
    }
    if(window)
    {
        SDL_DestroyWindow(window);
    }
}

void Application::set_viewport_mouse_capture(
  bool enabled)
{
    if(viewport_mouse_captured == enabled || window == nullptr)
    {
        return;
    }

    if(!SDL_SetWindowRelativeMouseMode(window, enabled))
    {
        logging::warningf(
          "failed to {} viewport mouse capture: {}",
          enabled ? "enable" : "disable",
          SDL_GetError());
        return;
    }

    viewport_mouse_captured = enabled;
    viewport_input.mouse_delta_x = 0.f;
    viewport_input.mouse_delta_y = 0.f;
}

void Application::update_viewport_mouse_capture()
{
    if(!viewport.is_editor_camera_modification_enabled()
       || !viewport.is_local_camera_active(scene))
    {
        set_viewport_mouse_capture(false);
        return;
    }

    float mouse_x = 0.f;
    float mouse_y = 0.f;
    const SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    const bool right_mouse_down = (mouse_buttons & SDL_BUTTON_RMASK) != 0;
    const bool should_capture =
      viewport_mouse_captured
        ? right_mouse_down
        : viewport_contains_mouse_position(viewport_input, mouse_x, mouse_y)
            && right_mouse_down;

    set_viewport_mouse_capture(should_capture);
}

void Application::run()
{
    bool running = true;
    int frame_index = 0;

    auto last_update_time = std::chrono::steady_clock::now();

    ImGuiIO& io = ImGui::GetIO();
    imgui::State ui_state;

    while(running)
    {
        const auto current_time = std::chrono::steady_clock::now();
        const float delta_time = std::chrono::duration<float>(
                                   current_time - last_update_time)
                                   .count();
        last_update_time = current_time;
        if(delta_time > 0.f)
        {
            tick(delta_time);
        }

        viewport_input.mouse_delta_x = 0.f;
        viewport_input.mouse_delta_y = 0.f;
        viewport_input.mouse_wheel_delta = 0.f;

        SDL_Event event;
        bool suppress_imgui_mouse = viewport_mouse_captured;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
               && event.button.button == SDL_BUTTON_RIGHT
               && viewport.is_editor_camera_modification_enabled()
               && viewport.is_local_camera_active(scene)
               && viewport_contains_mouse_position(
                 viewport_input,
                 event.button.x,
                 event.button.y))
            {
                set_viewport_mouse_capture(true);
            }
            suppress_imgui_mouse = suppress_imgui_mouse || viewport_mouse_captured;

            ImGui_ImplSDL3_ProcessEvent(&event);

            if(event.type == SDL_EVENT_QUIT)
            {
                set_viewport_mouse_capture(false);
                running = false;
            }
            else if(event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
            {
                set_viewport_mouse_capture(false);
            }
            else if(event.type == SDL_EVENT_MOUSE_BUTTON_UP
                    && event.button.button == SDL_BUTTON_RIGHT)
            {
                set_viewport_mouse_capture(false);
            }
            else if(event.type == SDL_EVENT_MOUSE_MOTION)
            {
                viewport_input.mouse_delta_x += event.motion.xrel;
                viewport_input.mouse_delta_y += event.motion.yrel;
            }
            else if(event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                viewport_input.mouse_wheel_delta += event.wheel.y;
            }
        }

        set_imgui_mouse_interactions_enabled(!suppress_imgui_mouse);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        imgui::draw_main_dockspace(running);
        imgui_draw_viewport_panel(
          render_device,
          renderer,
          scene,
          viewport,
          viewport_texture,
          running,
          viewport_input);
        imgui::draw_console_panel(log_device);
        imgui::draw_tools_panel(
          *this,
          render_device,
          viewport,
          scene,
          renderer,
          frame_index,
          pixel_density,
          io);

        // Check if benchmark was requested
        if(imgui::check_and_clear_sorting_benchmark_request())
        {
            renderer.start_sorting_benchmark(scene, viewport);
        }

        imgui::draw_scene_inspector_panel(ui_state, scene);
        imgui::draw_class_inspector_panel(ui_state);

        ImGui::Render();

        SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);
        glViewport(0, 0, pixel_w, pixel_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        ++frame_index;
    }
}

void Application::tick(float delta_time)
{
    update_viewport_mouse_capture();
    const ViewportNavigationMode navigation_mode = viewport.get_navigation_mode();
    const ViewportEditorCameraInput controller_input =
      viewport.is_editor_camera_modification_enabled()
        ? gather_viewport_camera_input(
            viewport_input,
            ImGui::GetIO(),
            viewport_mouse_captured,
            navigation_mode)
        : ViewportEditorCameraInput{};
    viewport.update_editor_camera(
      delta_time,
      controller_input);

    // FIXME temporary until a better update mechanism is in place
    scene.for_each_object<Gear>(
      [&](Gear& gear)
      {
          rebuild_gear_mesh_if_needed(render_device, &gear);
      });

    scene.tick(delta_time);
}

void Application::set_static_mesh_shader(StaticMeshShaderType type)
{
    active_static_mesh_shader = type;
    ShaderCache& shader_cache = renderer.get_shader_cache();

    scene.for_each_object<StaticMesh>(
      [&](StaticMesh& mesh)
      {
          // skip gears for now.
          if(mesh.is_a<Gear>())
          {
              return;
          }
          if(mesh.get_name() == floor_object_name)
          {
              return;
          }

          for(auto& lod: mesh.get_lods())
          {
              for(auto& section: lod.mesh_sections)
              {
                  render_device.delete_material(section.material_handle);

                  swr::program_base* new_shader = nullptr;
                  switch(type)
                  {
                  case StaticMeshShaderType::ColorFlat:
                      new_shader = shader_cache.get_or_create<shader::ColorFlat>();
                      break;
                  case StaticMeshShaderType::ColorSmooth:
                      new_shader = shader_cache.get_or_create<shader::ColorSmooth>();
                      break;
                  case StaticMeshShaderType::PhongSmooth:
                      new_shader = shader_cache.get_or_create<shader::PhongSmooth>();
                      break;
                  default:
                      throw std::runtime_error{"Unknown shader type for static meshes."};
                  }

                  section.material_handle = render_device.create_material(*new_shader);
              }
          }
      });
}

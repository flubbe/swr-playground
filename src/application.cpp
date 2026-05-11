/**
 * Software Rasterizer Playground.
 *
 * Main application.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <format>
#include <print>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "scene/gear.h"
#include "scene/scene.h"
#include "ui/imgui.h"
#include "application.h"
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

struct ViewportCameraControllerInput
{
    float move_forward{0.f};
    float move_right{0.f};
    float move_up{0.f};
    float look_yaw{0.f};
    float look_pitch{0.f};
    bool fast_move{false};
    bool active{false};
};

ml::mat4x4 make_view_matrix(
  const ViewportCameraControllerState& controller)
{
    ml::mat4x4 view = ml::mat4x4::identity();
    view *= ml::matrices::translation(
      -controller.position.x,
      -controller.position.y,
      -controller.position.z);
    view *= ml::matrices::rotation_x(controller.pitch_radians);
    view *= ml::matrices::rotation_y(controller.yaw_radians);
    return view;
}

ViewportCameraControllerInput gather_viewport_camera_input(
  const ViewportInputState& viewport_input,
  const ImGuiIO& io)
{
    ViewportCameraControllerInput input{};

    const SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(nullptr, nullptr);
    const bool right_mouse_down = (mouse_buttons & SDL_BUTTON_RMASK) != 0;
    if(!viewport_input.viewport_hovered || !right_mouse_down)
    {
        return input;
    }

    input.active = true;
    input.look_yaw = viewport_input.mouse_delta_x;
    input.look_pitch = viewport_input.mouse_delta_y;

    const bool keyboard_blocked = io.WantCaptureKeyboard;
    if(keyboard_blocked)
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

void update_viewport_camera_controller(
  ViewportCameraControllerState& controller,
  const ViewportCameraControllerInput& input,
  float delta_time)
{
    if(!input.active || delta_time <= 0.f)
    {
        return;
    }

    const float look_sensitivity = 0.0025f;
    controller.yaw_radians += input.look_yaw * look_sensitivity;
    controller.pitch_radians += input.look_pitch * look_sensitivity;
    controller.pitch_radians = std::clamp(
      controller.pitch_radians,
      ml::to_radians(-89.f),
      ml::to_radians(89.f));

    const float base_speed = input.fast_move ? 20.f : 8.f;
    const float move_distance = base_speed * delta_time;
    const float cos_pitch = std::cos(controller.pitch_radians);
    const float sin_pitch = std::sin(controller.pitch_radians);
    const float cos_yaw = std::cos(controller.yaw_radians);
    const float sin_yaw = std::sin(controller.yaw_radians);

    const ml::vec3 forward = {
      sin_yaw * cos_pitch,
      -sin_pitch,
      -cos_yaw * cos_pitch};
    const ml::vec3 right = {
      cos_yaw,
      0.f,
      sin_yaw};
    const ml::vec3 up = {0.f, 1.f, 0.f};

    controller.position += forward * (input.move_forward * move_distance);
    controller.position += right * (input.move_right * move_distance);
    controller.position += up * (input.move_up * move_distance);
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

void imgui_draw_viewport_panel(
  Application& app,
  RenderDevice& render_device,
  Renderer& renderer,
  Scene& scene,
  Viewport& viewport,
  GLuint& viewport_texture,
  std::vector<std::string>& log_lines,
  float& last_update_time,
  bool& running,
  bool& viewport_hovered)
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
            log_lines.push_back(
              std::format(
                "[info] resized viewport to {}x{}",
                render_device.get_width(),
                render_device.get_height()));
        }
        catch(const std::exception& e)
        {
            std::println(stderr, "{}", e.what());
            running = false;
        }
    }

    const float time_seconds = static_cast<float>(SDL_GetTicks()) / 1000.0f;
    const float delta_time = time_seconds - last_update_time;
    last_update_time = time_seconds;
    if(delta_time > 0)
    {
        app.tick(delta_time);
    }

    viewport.update_active_camera_projection(scene);

    renderer.render(
      scene,
      viewport);

    if(viewport_texture != 0)
    {
        update_viewport_texture(viewport_texture, render_device);

        // Display at logical UI size, not pixel size.
        ImGui::Image(
          static_cast<ImTextureID>(viewport_texture),
          avail,
          ImVec2{0, 0},
          ImVec2{1, 1});
        viewport_hovered = ImGui::IsItemHovered();

        if(viewport.is_camera_selector_overlay_enabled())
        {
            const ViewportCameraType camera_type = viewport.get_camera_type(scene);
            std::string camera_name = "Perspective";
            if(camera_type == ViewportCameraType::Scene)
            {
                camera_name = viewport.get_camera(scene).get_name();
            }

            const std::string label_left = "[";
            const std::string label_name = camera_name;
            const std::string label_right = "]";
            const std::string label = label_left + label_name + label_right;
            const ImVec2 viewport_min = ImGui::GetItemRectMin();
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
                if(ImGui::MenuItem("Perspective", nullptr, using_local_camera))
                {
                    viewport.use_local_camera();
                }

                ImGui::BeginDisabled(true);
                ImGui::MenuItem("Orthographic", nullptr, false);
                ImGui::EndDisabled();

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

                ImGui::EndPopup();
            }
        }
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

GearParameters create_gear_resources(
  RenderDevice& device,
  ShaderCache& shader_cache,
  const GearBuildParams& p)
{
    auto* flat_shader = shader_cache.add<shader::ColorFlat>(p.color);
    auto* smooth_shader = shader_cache.add<shader::ColorSmooth>(p.color);

    auto flat_material = device.create_material(*flat_shader);
    auto smooth_material = device.create_material(*smooth_shader);

    auto geom = make_gear(
      p.inner_radius,
      p.outer_radius,
      p.width,
      p.teeth,
      p.tooth_depth);

    auto inner_mesh = device.create_mesh(
      geom.inner_indices,
      geom.inner_vertices,
      geom.inner_normals);

    auto outer_mesh = device.create_mesh(
      geom.outer_indices,
      geom.outer_vertices,
      geom.outer_normals);

    return GearParameters{
      .inner = RenderData{
        .mesh_handle = inner_mesh,
        .material_handle = smooth_material,
      },
      .outer = RenderData{
        .mesh_handle = outer_mesh,
        .material_handle = flat_material,
      },
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

/** re-calculate the gear transformations. */
void update_gears(
  std::array<Gear*, 3>& gears,
  float time)
{
    gears[0]->set_transform(
      ml::matrices::translation(-3.f, -2.f, 0.f)
      * ml::matrices::rotation_z(time));
    gears[1]->set_transform(
      ml::matrices::translation(3.1f, -2.f, 0.f)
      * ml::matrices::rotation_z(-2.f * time - 9.f));
    gears[2]->set_transform(
      ml::matrices::translation(-3.1f, 4.2f, 0.f)
      * ml::matrices::rotation_z(-2.f * time - 25.f));
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

    const auto old_meshes = gear->get_meshes();
    if(old_meshes.size() != 2)
    {
        return;
    }

    auto geom = make_gear(
      gear->get_inner_radius(),
      gear->get_outer_radius(),
      gear->get_width(),
      teeth,
      gear->get_tooth_depth());

    const std::uint32_t old_inner_mesh = old_meshes[0].mesh_handle;
    const std::uint32_t old_outer_mesh = old_meshes[1].mesh_handle;

    const bool inner_updated = device.update_mesh(
      old_inner_mesh,
      std::move(geom.inner_indices),
      std::move(geom.inner_vertices),
      std::move(geom.inner_normals));
    const bool outer_updated = device.update_mesh(
      old_outer_mesh,
      std::move(geom.outer_indices),
      std::move(geom.outer_vertices),
      std::move(geom.outer_normals));

    if(!inner_updated || !outer_updated)
    {
        return;
    }

    gear->mark_rebuilt();
}

}    // namespace

void Application::setup_scene()
{
    if(!initialized)
    {
        throw std::runtime_error{"Application not initialized."};
    }

    struct GearInit
    {
        GearBuildParams build;
        ml::mat4x4 transform;
    };

    std::array<GearInit, 3> gears = {{
      {
        .build = {.color = {1, 0, 0, 1}, .inner_radius = 1.0f, .outer_radius = 4.0f, .width = 1.0f, .teeth = 20, .tooth_depth = 0.7f},
        .transform = ml::matrices::translation(-3.f, -2.f, 0.f),
      },
      {
        .build = {.color = {0, 1, 0, 1}, .inner_radius = 0.5f, .outer_radius = 2.0f, .width = 2.0f, .teeth = 10, .tooth_depth = 0.7f},
        .transform = ml::matrices::translation(3.1f, -2.f, 0.f),
      },
      {
        .build = {.color = {0, 0, 1, 1}, .inner_radius = 1.3f, .outer_radius = 2.0f, .width = 0.5f, .teeth = 10, .tooth_depth = 0.7f},
        .transform = ml::matrices::translation(-3.1f, 4.2f, 0.f),
      },
    }};

    GearFactory factory{*render_device, *shader_cache};

    for(std::size_t i = 0; i < gears.size(); ++i)
    {
        gear_objs[i] = &factory.create(*scene, gears[i].build, gears[i].transform);
    }

    // Local viewport camera stays the modifiable camera.
    viewport->get_local_camera().set_transform(
      make_view_matrix(viewport_camera_controller));

    Camera* camera = scene->add_object<Camera>();
    camera->set_transform(viewport->get_local_camera().get_transform());
    camera->set_name("Editor Camera");
    camera->capture_snapshot();

    viewport->use_local_camera();
}

void Application::setup_viewport()
{
    if(!initialized)
    {
        throw std::runtime_error{"Application not initialized."};
    }

    // Keep the local camera in sync as a fallback if the bound scene camera is removed.
    viewport->get_local_camera().set_transform(
      make_view_matrix(viewport_camera_controller));
}

Application::Application(
  std::string_view title)
: title{title}
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

    std::println("display scale: {}", display_scale);
    std::println("pixel density: {}", pixel_density);
    std::println("window size: {} x {}", window_w, window_h);
    std::println("pixel size: {} x {}", pixel_w, pixel_h);
}

Application::~Application()
{
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

void Application::initialize(
  RenderDevice& render_device,
  ShaderCache& shader_cache,
  Renderer& renderer,
  Scene& scene,
  Viewport& viewport)
{
    if(initialized)
    {
        throw std::runtime_error{
          "Application already initialized."};
    }
    initialized = true;

    this->render_device = &render_device;
    this->shader_cache = &shader_cache;
    this->renderer = &renderer;
    this->scene = &scene;
    this->viewport = &viewport;

    viewport_texture = create_viewport_texture(
      render_device.get_width(),
      render_device.get_height());
    viewport.set_resolution(
      render_device.get_width(),
      render_device.get_height());

    setup_scene();
    scene.add_default_systems();
    setup_viewport();
}

void Application::run()
{
    if(!initialized)
    {
        throw std::runtime_error{"Application not initialized."};
    }

    std::vector<std::string> log_lines = {
      "[info] editor started",
      "[info] dock layout initialized",
      "[info] viewport ready"};

    bool running = true;
    int frame_index = 0;

    float last_update_time = static_cast<float>(SDL_GetTicks()) / 1000.0f;

    ImGuiIO& io = ImGui::GetIO();
    imgui::State ui_state;

    while(running)
    {
        viewport_input.mouse_delta_x = 0.f;
        viewport_input.mouse_delta_y = 0.f;

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if(event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if(event.type == SDL_EVENT_MOUSE_MOTION)
            {
                viewport_input.mouse_delta_x += event.motion.xrel;
                viewport_input.mouse_delta_y += event.motion.yrel;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        imgui::draw_main_dockspace(running);
        imgui_draw_viewport_panel(
          *this,
          *render_device,
          *renderer,
          *scene,
          *viewport,
          viewport_texture,
          log_lines,
          last_update_time,
          running,
          viewport_input.viewport_hovered);
        imgui::draw_console_panel(log_lines);
        imgui::draw_tools_panel(
          *render_device,
          *viewport,
          *scene,
          *renderer,
          frame_index,
          pixel_density,
          io);
        imgui::draw_scene_inspector_panel(ui_state, *scene);
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
    const ViewportCameraControllerInput controller_input =
      gather_viewport_camera_input(viewport_input, ImGui::GetIO());
    update_viewport_camera_controller(
      viewport_camera_controller,
      controller_input,
      delta_time);
    viewport->get_local_camera().set_transform(
      make_view_matrix(viewport_camera_controller));

    scene->tick(delta_time);
    for(auto* gear: gear_objs)
    {
        rebuild_gear_mesh_if_needed(*render_device, gear);
    }
    update_gears(gear_objs, scene->get_time());
}

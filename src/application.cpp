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

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "scene/gear.h"
#include "scene/scene.h"
#include "application.h"
#include "imgui.h"
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

void setup_dock_layout(ImGuiID dockspace_id)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(
      dockspace_id,
      ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

    ImGuiID dock_main = dockspace_id;
    ImGuiID dock_left = 0;
    ImGuiID dock_right = 0;
    ImGuiID dock_bottom = 0;

    dock_left = ImGui::DockBuilderSplitNode(
      dock_main,
      ImGuiDir_Left,
      0.20f,
      nullptr,
      &dock_main);

    dock_right = ImGui::DockBuilderSplitNode(
      dock_main,
      ImGuiDir_Right,
      0.25f,
      nullptr,
      &dock_main);

    dock_bottom = ImGui::DockBuilderSplitNode(
      dock_main,
      ImGuiDir_Down,
      0.25f,
      nullptr,
      &dock_main);

    ImGui::DockBuilderDockWindow("Viewport", dock_main);
    ImGui::DockBuilderDockWindow("Console", dock_bottom);
    ImGui::DockBuilderDockWindow("Tools", dock_right);
    ImGui::DockBuilderDockWindow("Inspector", dock_left);

    ImGui::DockBuilderFinish(dockspace_id);
}

void draw_main_dockspace(
  bool& running)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiWindowFlags host_window_flags =
      ImGuiWindowFlags_NoDocking
      | ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoCollapse
      | ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoBringToFrontOnFocus
      | ImGuiWindowFlags_NoNavFocus
      | ImGuiWindowFlags_MenuBar;

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});

    ImGui::Begin("MainDockHost", nullptr, host_window_flags);
    ImGui::PopStyleVar(3);

    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Quit", nullptr, false, true))
            {
                running = false;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2{0.0f, 0.0f});

    static bool first_time = true;
    if(first_time)
    {
        setup_dock_layout(dockspace_id);
        first_time = false;
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
  std::array<Object*, 3>& gears,
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

void imgui_draw_console(std::vector<std::string>& log_lines)
{
    ImGui::Begin("Console");

    if(ImGui::Button("Clear"))
    {
        log_lines.clear();
    }

    ImGui::Separator();

    ImGui::BeginChild(
      "ConsoleScrollRegion",
      ImVec2{0, 0},
      false,
      ImGuiWindowFlags_HorizontalScrollbar);
    for(const std::string& line: log_lines)
    {
        ImGui::TextUnformatted(line.c_str());
    }
    if(ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

void imgui_draw_tools(
  ImGuiIO& io,
  RenderDevice& render_device,
  Renderer& renderer,
  Scene& scene,
  Viewport& viewport,
  float pixel_density,
  int frame_index)
{
    ImGui::Begin("Tools");

    if(ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text(
          "Framebuffer: %d x %d px",
          render_device.get_width(),
          render_device.get_height());
        ImGui::Text("Window pixel density: %.2f", pixel_density);
        ImGui::Text("Frame: %d", frame_index);
        ImGui::Text("Scene time: %.1f s", scene.get_time());
    }

    if(ImGui::CollapsingHeader("Rasterizer", ImGuiTreeNodeFlags_DefaultOpen))
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

    if(ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("ms/frame: %.3f", 1000.0f / std::max(io.Framerate, 0.001f));
        ImGui::Text("render time: %.3f ms", 1000.f * renderer.get_render_time());
    }

    ImGui::End();
}

void imgui_draw_scene_inspector(
  Scene& scene,
  std::optional<ObjectId>& selected_object_id)
{
    ImGui::Begin("Inspector");

    for(const auto& obj_ptr: scene.get_objects())
    {
        if(!obj_ptr)
        {
            continue;
        }

        Object& obj = *obj_ptr;
        bool is_selected = (selected_object_id == obj.get_object_id());

        const auto type_name = obj.get_class()->name;
        std::string label =
          std::format(
            "{} ({})##{}",
            obj.get_name(),
            type_name,
            obj.get_object_id().value);

        ImGuiTreeNodeFlags flags =
          ImGuiTreeNodeFlags_OpenOnArrow
          | ImGuiTreeNodeFlags_OpenOnDoubleClick
          | ImGuiTreeNodeFlags_SpanAvailWidth;

        if(is_selected)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool open = ImGui::TreeNodeEx(label.c_str(), flags);

        if(ImGui::IsItemClicked())
        {
            selected_object_id = obj.get_object_id();
        }

        if(open)
        {
            ImGui::Text("ID: %u", obj.get_object_id().value);
            ImGui::TreePop();
        }
    }

    ImGui::End();
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
}

void Application::setup_viewport()
{
    if(!initialized)
    {
        throw std::runtime_error{"Application not initialized."};
    }

    ml::mat4x4 view = ml::mat4x4::identity();
    view *= ml::matrices::translation(0.f, 0.f, -40.f);

    ml::vec3 view_rotation = {20.f, 30.f, 0.f};
    view *= ml::matrices::rotation_x(ml::to_radians(view_rotation.x));
    view *= ml::matrices::rotation_y(ml::to_radians(view_rotation.y));
    view *= ml::matrices::rotation_z(ml::to_radians(view_rotation.z));

    viewport->camera.set_transform(view);
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

    if(!imgui_init(window, gl_context))
    {
        throw std::runtime_error{"imgui_init failed."};
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
    imgui_shutdown();

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

    setup_scene();
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

    while(running)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if(event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        draw_main_dockspace(running);

        ImGui::Begin("Viewport");

        ImVec2 avail = ImGui::GetContentRegionAvail();

        // ImGui sizes are logical units; rasterizer target should use pixels.
        int viewport_w_px = std::max(1, static_cast<int>(std::round(avail.x * pixel_density)));
        int viewport_h_px = std::max(1, static_cast<int>(std::round(avail.y * pixel_density)));

        // FIXME the dimensions should not come from the render device
        if(viewport_w_px != render_device->get_width()
           || viewport_h_px != render_device->get_height())
        {
            viewport->camera.set_resolution(viewport_w_px, viewport_h_px);
            render_device->resize(viewport_w_px, viewport_h_px);

            destroy_viewport_texture(viewport_texture);

            try
            {
                viewport_texture = create_viewport_texture(
                  render_device->get_width(),
                  render_device->get_height());
                log_lines.push_back(
                  std::format(
                    "[info] resized viewport to {}x{}",
                    render_device->get_width(),
                    render_device->get_height()));
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
            tick(delta_time);
        }

        renderer->render(
          *scene,
          *viewport);

        if(viewport_texture != 0)
        {
            update_viewport_texture(viewport_texture, *render_device);

            // Display at logical UI size, not pixel size.
            ImGui::Image(
              static_cast<ImTextureID>(viewport_texture),
              avail,
              ImVec2{0, 0},
              ImVec2{1, 1});
        }

        ImGui::End();

        imgui_draw_console(log_lines);
        imgui_draw_tools(
          io,
          *render_device,
          *renderer,
          *scene,
          *viewport,
          pixel_density,
          frame_index);
        imgui_draw_scene_inspector(*scene, selected_object_id);

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
    scene->tick(delta_time);
    update_gears(gear_objs, scene->get_time());
}
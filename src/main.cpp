/**
 * Software Rasterizer Playground.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cmath>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include <gsl/gsl>

#include "swr/swr.h"

#include "platform.h"
#include "scene/scene.h"
#include "renderdevice.h"
#include "renderer.h"
#include "shader_cache.h"
#include "viewport.h"

namespace
{

class SDLError : public std::runtime_error
{
public:
    explicit SDLError(std::string_view message)
    : std::runtime_error{
        std::format(
          "{}: {}",
          message,
          SDL_GetError())}
    {
    }
};

class GLError : public std::runtime_error
{
public:
    explicit GLError(std::string_view message)
    : std::runtime_error{std::string{message}}
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
        throw GLError{"glGenTextures failed"};
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
      GL_RGBA8,
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
    ImGuiID dock_right = 0;
    ImGuiID dock_bottom = 0;

    dock_right = ImGui::DockBuilderSplitNode(
      dock_main,
      ImGuiDir_Right,
      0.25f,
      nullptr,
      &dock_main);
    dock_bottom = ImGui::DockBuilderSplitNode(
      dock_main,
      ImGuiDir_Down,
      0.25f, nullptr,
      &dock_main);

    ImGui::DockBuilderDockWindow("Viewport", dock_main);
    ImGui::DockBuilderDockWindow("Console", dock_bottom);
    ImGui::DockBuilderDockWindow("Tools", dock_right);

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

}    // namespace

int main(int, char**)
{
    if(!platform_init())
    {
        return EXIT_FAILURE;
    }

    const auto shutdown = gsl::finally([]() -> void
                                       { platform_shutdown(); });

    SDL_Window* window = SDL_CreateWindow(
      "SWR Playground",
      1280,
      800,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if(!window)
    {
        std::println(stderr, "SDL_CreateWindow failed: {}", SDL_GetError());
        return 1;
    }

    const auto window_cleanup = gsl::finally(
      [&]() -> void
      {
        if(window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        } });

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if(!gl_context)
    {
        std::println(stderr, "SDL_GL_CreateContext failed: {}", SDL_GetError());
        return 1;
    }

    const auto gl_cleanup = gsl::finally(
      [&]
      {
        if(gl_context)
        {
            SDL_GL_DestroyContext(gl_context);
            gl_context = nullptr;
        } });

    if(!SDL_GL_MakeCurrent(window, gl_context))
    {
        std::println(stderr, "SDL_GL_MakeCurrent failed: {}", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_GL_SetSwapInterval(1);

    if(!imgui_init(window, gl_context))
    {
        std::println(stderr, "imgui_init failed.");
        return EXIT_FAILURE;
    }

    const auto imgui_cleanup = gsl::finally(
      []
      {
          imgui_shutdown();
      });

    float pixel_density = SDL_GetWindowPixelDensity(window);
    float display_scale = SDL_GetWindowDisplayScale(window);
    int window_w = 0, window_h = 0;
    SDL_GetWindowSize(window, &window_w, &window_h);
    int pixel_w = 0, pixel_h = 0;
    SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);

    std::println("display scale: {}", display_scale);
    std::println("pixel density: {}", pixel_density);
    std::println("window size: {} x {}", window_w, window_h);
    std::println("pixel size: {} x {}", pixel_w, pixel_h);

    RenderDevice render_device{640, 480};
    ShaderCache shader_cache;
    Camera cam{
      render_device.get_width(),
      render_device.get_height()};
    Renderer renderer{render_device};

    Scene scene;
    Viewport viewport;

    // create materials.
    std::array<shader::ColorFlat*, 3> flat_shaders = {
      shader_cache.add<shader::ColorFlat>(ml::vec4{1, 0, 0, 1}),
      shader_cache.add<shader::ColorFlat>(ml::vec4{0, 1, 0, 1}),
      shader_cache.add<shader::ColorFlat>(ml::vec4{0, 0, 1, 1})};

    std::array<shader::ColorSmooth*, 3> smooth_shaders = {
      shader_cache.add<shader::ColorSmooth>(ml::vec4{1, 0, 0, 1}),
      shader_cache.add<shader::ColorSmooth>(ml::vec4{0, 1, 0, 1}),
      shader_cache.add<shader::ColorSmooth>(ml::vec4{0, 0, 1, 1})};

    std::array<std::uint32_t, 3> flat_material_handles = {
      render_device.create_material(*flat_shaders[0]),
      render_device.create_material(*flat_shaders[1]),
      render_device.create_material(*flat_shaders[2]),
    };
    std::array<std::uint32_t, 3> smooth_material_handles = {
      render_device.create_material(*smooth_shaders[0]),
      render_device.create_material(*smooth_shaders[1]),
      render_device.create_material(*smooth_shaders[2])};

    std::array<Object*, 3> gear_objs = {nullptr, nullptr, nullptr};
    {
        std::array<GearGeometry, 3> gear_geoms = {
          make_gear(1.0, 4.0, 1.0, 20, 0.7),
          make_gear(0.5, 2.0, 2.0, 10, 0.7),
          make_gear(1.3, 2.0, 0.5, 10, 0.7)};

        std::array<std::uint32_t, 3> inner_mesh_handles = {
          render_device.create_mesh(
            gear_geoms[0].inner_indices,
            gear_geoms[0].inner_vertices,
            gear_geoms[0].inner_normals),
          render_device.create_mesh(
            gear_geoms[1].inner_indices,
            gear_geoms[1].inner_vertices,
            gear_geoms[1].inner_normals),
          render_device.create_mesh(
            gear_geoms[2].inner_indices,
            gear_geoms[2].inner_vertices,
            gear_geoms[2].inner_normals)};

        std::array<std::uint32_t, 3> outer_mesh_handles = {
          render_device.create_mesh(
            gear_geoms[0].outer_indices,
            gear_geoms[0].outer_vertices,
            gear_geoms[0].outer_normals),
          render_device.create_mesh(
            gear_geoms[1].outer_indices,
            gear_geoms[1].outer_vertices,
            gear_geoms[1].outer_normals),
          render_device.create_mesh(
            gear_geoms[2].outer_indices,
            gear_geoms[2].outer_vertices,
            gear_geoms[2].outer_normals)};

        std::array<ml::mat4x4, 3> transforms = {
          ml::matrices::translation(-3.f, -2.f, 0.f),
          ml::matrices::translation(3.1f, -2.f, 0.f),
          ml::matrices::translation(-3.1f, 4.2f, 0.f)};

        // populate scene.
        for(std::size_t i = 0; i < 3; ++i)
        {
            gear_objs[i] = scene.add_object<Object>(
              std::vector{
                MeshHandle{
                  .mesh_handle = inner_mesh_handles[i],
                  .material_handle = smooth_material_handles[i]},
                MeshHandle{
                  .mesh_handle = outer_mesh_handles[i],
                  .material_handle = flat_material_handles[i]}});

            gear_objs[i]->set_transform(transforms[i]);
        }
    }

    // TODO set up light
    ml::mat4x4 view = ml::mat4x4::identity();
    view *= ml::matrices::translation(0.f, 0.f, -40.f);

    ml::vec3 view_rotation = {20.f, 30.f, 0.f};
    view *= ml::matrices::rotation_x(ml::to_radians(view_rotation.x));
    view *= ml::matrices::rotation_y(ml::to_radians(view_rotation.y));
    view *= ml::matrices::rotation_z(ml::to_radians(view_rotation.z));

    viewport.camera.set_transform(view);

    GLuint viewport_texture = 0;
    try
    {
        viewport_texture = create_viewport_texture(
          render_device.get_width(),
          render_device.get_height());
    }
    catch(const std::exception& e)
    {
        std::println(stderr, "{}", e.what());
        return 1;
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
        if(viewport_w_px != render_device.get_width()
           || viewport_h_px != render_device.get_height())
        {
            viewport.camera.set_resolution(viewport_w_px, viewport_h_px);
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
            scene.tick(delta_time);
            update_gears(gear_objs, scene.get_time());
        }

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
        }

        ImGui::End();

        ImGui::Begin("Console");

        if(ImGui::Button("Clear"))
        {
            log_lines.clear();
        }

        ImGui::Separator();

        ImGui::BeginChild("ConsoleScrollRegion", ImVec2{0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);
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

        ImGui::Render();

        SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);
        glViewport(0, 0, pixel_w, pixel_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        ++frame_index;
    }

    destroy_viewport_texture(viewport_texture);

    return 0;
}
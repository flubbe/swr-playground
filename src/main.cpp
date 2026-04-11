/**
 * Software Rasterizer Playground.
 *
 * color shader with directional lighting.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "swr/swr.h"

#include "framebuffer.h"

namespace
{

class sdl_error : public std::runtime_error
{
public:
    explicit sdl_error(std::string_view message)
    : std::runtime_error{std::format("{}: {}", message, SDL_GetError())}
    {
    }
};

class gl_error : public std::runtime_error
{
public:
    explicit gl_error(std::string_view message)
    : std::runtime_error{std::string{message}}
    {
    }
};

GLuint create_viewport_texture(int width, int height)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    if(texture == 0)
    {
        throw gl_error{"glGenTextures failed"};
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

void update_viewport_texture(GLuint texture, const Framebuffer& framebuffer)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
      GL_TEXTURE_2D,
      0,
      0,
      0,
      framebuffer.get_width(),
      framebuffer.get_height(),
      GL_BGRA,
      GL_UNSIGNED_INT_8_8_8_8_REV,
      framebuffer.get_data());
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

void apply_editor_theme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2{10.0f, 10.0f};
    style.FramePadding = ImVec2{8.0f, 6.0f};
    style.CellPadding = ImVec2{8.0f, 4.0f};
    style.ItemSpacing = ImVec2{8.0f, 8.0f};
    style.ItemInnerSpacing = ImVec2{6.0f, 6.0f};
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;

    style.Colors[ImGuiCol_Text] = {0.92f, 0.93f, 0.94f, 1.00f};
    style.Colors[ImGuiCol_TextDisabled] = {0.50f, 0.54f, 0.58f, 1.00f};
    style.Colors[ImGuiCol_WindowBg] = {0.10f, 0.105f, 0.11f, 1.00f};
    style.Colors[ImGuiCol_ChildBg] = {0.12f, 0.125f, 0.13f, 1.00f};
    style.Colors[ImGuiCol_PopupBg] = {0.12f, 0.125f, 0.13f, 0.98f};
    style.Colors[ImGuiCol_Border] = {0.24f, 0.25f, 0.29f, 1.00f};
    style.Colors[ImGuiCol_BorderShadow] = {0.00f, 0.00f, 0.00f, 0.00f};
    style.Colors[ImGuiCol_FrameBg] = {0.16f, 0.17f, 0.19f, 1.00f};
    style.Colors[ImGuiCol_FrameBgHovered] = {0.22f, 0.23f, 0.27f, 1.00f};
    style.Colors[ImGuiCol_FrameBgActive] = {0.28f, 0.29f, 0.34f, 1.00f};
    style.Colors[ImGuiCol_TitleBg] = {0.09f, 0.095f, 0.10f, 1.00f};
    style.Colors[ImGuiCol_TitleBgActive] = {0.13f, 0.14f, 0.16f, 1.00f};
    style.Colors[ImGuiCol_TitleBgCollapsed] = {0.09f, 0.095f, 0.10f, 1.00f};
    style.Colors[ImGuiCol_MenuBarBg] = {0.14f, 0.145f, 0.15f, 1.00f};
    style.Colors[ImGuiCol_ScrollbarBg] = {0.10f, 0.105f, 0.11f, 1.00f};
    style.Colors[ImGuiCol_ScrollbarGrab] = {0.25f, 0.27f, 0.30f, 1.00f};
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = {0.31f, 0.34f, 0.38f, 1.00f};
    style.Colors[ImGuiCol_ScrollbarGrabActive] = {0.38f, 0.41f, 0.46f, 1.00f};
    style.Colors[ImGuiCol_CheckMark] = {0.70f, 0.78f, 0.96f, 1.00f};
    style.Colors[ImGuiCol_SliderGrab] = {0.60f, 0.67f, 0.84f, 1.00f};
    style.Colors[ImGuiCol_SliderGrabActive] = {0.70f, 0.78f, 0.96f, 1.00f};
    style.Colors[ImGuiCol_Button] = {0.20f, 0.22f, 0.25f, 1.00f};
    style.Colors[ImGuiCol_ButtonHovered] = {0.27f, 0.30f, 0.34f, 1.00f};
    style.Colors[ImGuiCol_ButtonActive] = {0.32f, 0.35f, 0.40f, 1.00f};
    style.Colors[ImGuiCol_Header] = {0.20f, 0.22f, 0.25f, 1.00f};
    style.Colors[ImGuiCol_HeaderHovered] = {0.27f, 0.30f, 0.34f, 1.00f};
    style.Colors[ImGuiCol_HeaderActive] = {0.32f, 0.35f, 0.40f, 1.00f};
    style.Colors[ImGuiCol_Separator] = {0.24f, 0.25f, 0.29f, 1.00f};
    style.Colors[ImGuiCol_SeparatorHovered] = {0.40f, 0.44f, 0.50f, 1.00f};
    style.Colors[ImGuiCol_SeparatorActive] = {0.52f, 0.57f, 0.65f, 1.00f};
    style.Colors[ImGuiCol_ResizeGrip] = {0.24f, 0.25f, 0.29f, 0.20f};
    style.Colors[ImGuiCol_ResizeGripHovered] = {0.52f, 0.57f, 0.65f, 0.67f};
    style.Colors[ImGuiCol_ResizeGripActive] = {0.70f, 0.78f, 0.96f, 0.95f};
    style.Colors[ImGuiCol_Tab] = {0.14f, 0.145f, 0.15f, 1.00f};
    style.Colors[ImGuiCol_TabHovered] = {0.28f, 0.30f, 0.34f, 1.00f};
    style.Colors[ImGuiCol_TabActive] = {0.20f, 0.22f, 0.25f, 1.00f};
    style.Colors[ImGuiCol_TabUnfocused] = {0.12f, 0.125f, 0.13f, 1.00f};
    style.Colors[ImGuiCol_TabUnfocusedActive] = {0.16f, 0.17f, 0.19f, 1.00f};
    style.Colors[ImGuiCol_DockingPreview] = {0.70f, 0.78f, 0.96f, 0.70f};
    style.Colors[ImGuiCol_DockingEmptyBg] = {0.10f, 0.105f, 0.11f, 1.00f};
}

void load_fonts()
{
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;

    io.Fonts->Clear();
    ImFont* font = io.Fonts->AddFontFromFileTTF(
      "assets/fonts/inter/Inter-Regular.ttf",
      16.0f,
      &cfg);
    if(font == nullptr)
    {
        throw std::runtime_error{"Unable to load font."};
    }

    io.FontGlobalScale = 1.0f;
}

}    // namespace

int main(int, char**)
{
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Attributes should be set before context creation.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
      "SWR Playground",
      1280,
      800,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if(!window)
    {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if(!gl_context)
    {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if(!SDL_GL_MakeCurrent(window, gl_context))
    {
        std::fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    load_fonts();
    apply_editor_theme();

    // Backend init: SDL platform + OpenGL renderer.
    if(!ImGui_ImplSDL3_InitForOpenGL(window, gl_context))
    {
        std::fprintf(stderr, "ImGui_ImplSDL3_InitForOpenGL failed\n");
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // GLSL version string should match your context.
    if(!ImGui_ImplOpenGL3_Init("#version 330"))
    {
        std::fprintf(stderr, "ImGui_ImplOpenGL3_Init failed\n");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    float pixel_density = SDL_GetWindowPixelDensity(window);
    float display_scale = SDL_GetWindowDisplayScale(window);
    int window_w = 0, window_h = 0;
    SDL_GetWindowSize(window, &window_w, &window_h);
    int pixel_w = 0, pixel_h = 0;
    SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);

    std::printf("display scale: %f\n", display_scale);
    std::printf("pixel density: %f\n", pixel_density);
    std::printf("window size: %d x %d\n", window_w, window_h);
    std::printf("pixel size: %d x %d\n", pixel_w, pixel_h);

    Framebuffer framebuffer(640, 480);

    GLuint viewport_texture = 0;
    try
    {
        viewport_texture = create_viewport_texture(framebuffer.get_width(), framebuffer.get_height());
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<std::string> log_lines = {
      "[info] editor started",
      "[info] dock layout initialized",
      "[info] viewport ready"};

    bool running = true;
    bool viewport_hovered = false;
    int frame_index = 0;

    float last_update_time = static_cast<float>(SDL_GetTicks()) / 1000.0f;

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

        if(viewport_w_px != framebuffer.get_width() || viewport_h_px != framebuffer.get_height())
        {
            framebuffer.resize(viewport_w_px, viewport_h_px);
            destroy_viewport_texture(viewport_texture);

            try
            {
                viewport_texture = create_viewport_texture(framebuffer.get_width(), framebuffer.get_height());
                log_lines.push_back(
                  std::format(
                    "[info] resized viewport to {}x{}",
                    framebuffer.get_width(),
                    framebuffer.get_height()));
            }
            catch(const std::exception& e)
            {
                std::println(stderr, "%s", e.what());
                running = false;
            }
        }

        const float time_seconds = static_cast<float>(SDL_GetTicks()) / 1000.0f;
        const float delta_time = time_seconds - last_update_time;
        last_update_time = time_seconds;
        if(delta_time > 0)
        {
            framebuffer.update(delta_time);
        }

        if(viewport_texture != 0)
        {
            update_viewport_texture(viewport_texture, framebuffer);
            viewport_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

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
            ImGui::Text("Framebuffer: %d x %d px", framebuffer.get_width(), framebuffer.get_height());
            ImGui::Text("Window pixel density: %.2f", pixel_density);
            ImGui::Text("Frame: %d", frame_index);
        }

        if(ImGui::CollapsingHeader("Rasterizer", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static bool wireframe = false;
            static bool cull_face = true;
            static bool update_animation = true;

            if(ImGui::Checkbox("Animate", &update_animation))
            {
                framebuffer.set_update_animation(update_animation);
            }

            if(ImGui::Checkbox("Wireframe", &wireframe))
            {
                framebuffer.set_wireframe(wireframe);
            }

            if(ImGui::Checkbox("Face Culling", &cull_face))
            {
                framebuffer.set_cull_face(cull_face);
            }
        }

        if(ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("ms/frame: %.3f", 1000.0f / std::max(io.Framerate, 0.001f));
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
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
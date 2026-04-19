#include <print>
#include <stdexcept>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "imgui.h"

namespace
{

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

bool imgui_init(
  SDL_Window* window,
  SDL_GLContext gl_context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    load_fonts();
    apply_editor_theme();

    if(!ImGui_ImplSDL3_InitForOpenGL(window, gl_context))
    {
        std::println(stderr, "ImGui_ImplSDL3_InitForOpenGL failed");
        ImGui::DestroyContext();
        return false;
    }

    if(!ImGui_ImplOpenGL3_Init("#version 330"))
    {
        std::println(stderr, "ImGui_ImplOpenGL3_Init failed");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    return true;
}

void imgui_shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
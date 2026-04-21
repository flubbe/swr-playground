#include <algorithm>
#include <cmath>
#include <format>
#include <print>
#include <stdexcept>

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "imgui.h"
#include "renderdevice.h"
#include "renderer.h"
#include "scene/scene.h"
#include "viewport.h"

namespace
{
Object* g_selected_object = nullptr;    // FIXME should not be global.

void validate_selected_object(Scene& scene) noexcept
{
    if(g_selected_object == nullptr)
    {
        return;
    }

    const auto& objects = scene.get_objects();
    const bool found = std::any_of(
      objects.begin(),
      objects.end(),
      [](const std::unique_ptr<Object>& object)
      {
          return object.get() == g_selected_object;
      });
    if(!found)
    {
        g_selected_object = nullptr;
    }
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

void imgui_setup_dock_layout(ImGuiID dockspace_id)
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
      0.22f,
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
      0.25f, nullptr,
      &dock_main);

    ImGui::DockBuilderDockWindow("Viewport", dock_main);
    ImGui::DockBuilderDockWindow("Console", dock_bottom);
    ImGui::DockBuilderDockWindow("Tools", dock_right);
    ImGui::DockBuilderDockWindow("Inspector", dock_left);

    ImGui::DockBuilderFinish(dockspace_id);
}

class ImGuiPropertyRenderer : public PropertyVisitor
{
public:
    void visit(IntProperty& property) override
    {
        if(!property.has_value())
        {
            ImGui::TextUnformatted("<null>");
            return;
        }

        if(property.is_read_only())
        {
            ImGui::Text("%d", property.get_value());
            return;
        }

        int value = property.get_value();
        const bool changed = property.has_limits_enabled()
                               ? ImGui::DragInt(
                                   "##value",
                                   &value,
                                   property.get_speed(),
                                   property.get_min_value(),
                                   property.get_max_value())
                               : ImGui::DragInt(
                                   "##value",
                                   &value,
                                   property.get_speed());
        if(changed)
        {
            property.set_value(value);
        }
    }

    void visit(UIntProperty& property) override
    {
        if(!property.has_value())
        {
            ImGui::TextUnformatted("<null>");
            return;
        }

        if(property.is_read_only())
        {
            ImGui::Text("%u", property.get_value());
            return;
        }

        std::uint32_t value = property.get_value();
        const std::uint32_t min_value = property.get_min_value();
        const std::uint32_t max_value = property.get_max_value();
        const bool changed = property.has_limits_enabled()
                               ? ImGui::DragScalar(
                                   "##value",
                                   ImGuiDataType_U32,
                                   &value,
                                   property.get_speed(),
                                   &min_value,
                                   &max_value,
                                   "%u")
                               : ImGui::DragScalar(
                                   "##value",
                                   ImGuiDataType_U32,
                                   &value,
                                   property.get_speed(),
                                   nullptr,
                                   nullptr,
                                   "%u");
        if(changed)
        {
            property.set_value(value);
        }
    }

    void visit(FloatProperty& property) override
    {
        if(!property.has_value())
        {
            ImGui::TextUnformatted("<null>");
            return;
        }

        if(property.is_read_only())
        {
            ImGui::Text(property.get_format(), property.get_value());
            return;
        }

        float value = property.get_value();
        const bool changed = property.has_limits_enabled()
                               ? ImGui::DragFloat(
                                   "##value",
                                   &value,
                                   property.get_speed(),
                                   property.get_min_value(),
                                   property.get_max_value(),
                                   property.get_format())
                               : ImGui::DragFloat(
                                   "##value",
                                   &value,
                                   property.get_speed(),
                                   0.0f,
                                   0.0f,
                                   property.get_format());
        if(changed)
        {
            property.set_value(value);
        }
    }

    void visit(BoolProperty& property) override
    {
        if(!property.has_value())
        {
            ImGui::TextUnformatted("<null>");
            return;
        }

        if(property.is_read_only())
        {
            ImGui::TextUnformatted(property.get_value() ? "true" : "false");
            return;
        }

        bool value = property.get_value();
        if(ImGui::Checkbox("##value", &value))
        {
            property.set_value(value);
        }
    }

    void visit(StringProperty& property) override
    {
        if(!property.has_value())
        {
            ImGui::TextUnformatted("<null>");
            return;
        }

        if(property.is_read_only())
        {
            ImGui::TextUnformatted(property.get_value().c_str());
            return;
        }

        const std::size_t buffer_size = std::max<std::size_t>(property.get_max_length() + 1, 2);
        std::vector<char> buffer(buffer_size, '\0');
        const std::string& value = property.get_value();
        const std::size_t copied = std::min(value.size(), property.get_max_length());
        std::copy_n(value.data(), copied, buffer.data());
        buffer[copied] = '\0';

        if(ImGui::InputText("##value", buffer.data(), buffer.size()))
        {
            property.set_value(buffer.data());
        }
    }
};

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

void imgui_draw_main_dockspace(bool& running)
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
        imgui_setup_dock_layout(dockspace_id);
        first_time = false;
    }

    ImGui::End();
}

void imgui_draw_console_panel(std::vector<std::string>& log_lines)
{
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
}

void imgui_draw_tools_panel(
  RenderDevice& render_device,
  Viewport& viewport,
  Scene& scene,
  Renderer& renderer,
  int frame_index,
  float pixel_density,
  const ImGuiIO& io)
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

void imgui_draw_inspector_panel(Scene& scene)
{
    ImGui::Begin("Inspector");
    validate_selected_object(scene);

    auto& objects = scene.get_objects();
    if(objects.empty())
    {
        ImGui::TextUnformatted("No objects in scene.");
    }
    else
    {
        for(auto& object: objects)
        {
            Object* inspected = object.get();
            const ClassInfo* class_info = inspected->get_class();
            const std::string type_name = class_info != nullptr
                                            ? std::string{class_info->name}
                                            : std::string{"Unknown"};
            const std::string object_header = std::format(
              "{} ({})##{}",
              inspected->get_name(),
              type_name,
              inspected->get_object_id().value);

            ImGuiTreeNodeFlags header_flags =
              ImGuiTreeNodeFlags_DefaultOpen
              | ImGuiTreeNodeFlags_SpanAvailWidth;
            if(inspected == g_selected_object)
            {
                header_flags |= ImGuiTreeNodeFlags_Selected;
            }

            if(ImGui::CollapsingHeader(object_header.c_str(), header_flags))
            {
                const std::string table_id = std::format(
                  "ObjectProperties##{}",
                  inspected->get_object_id().value);
                const ImGuiTableFlags table_flags =
                  ImGuiTableFlags_BordersInnerV
                  | ImGuiTableFlags_BordersOuter
                  | ImGuiTableFlags_RowBg
                  | ImGuiTableFlags_SizingStretchSame;

                if(ImGui::BeginTable(table_id.c_str(), 2, table_flags))
                {
                    ImGui::TableSetupColumn("Property");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableHeadersRow();

                    auto& properties = inspected->get_properties();
                    ImGuiPropertyRenderer property_renderer;
                    for(std::size_t i = 0; i < properties.size(); ++i)
                    {
                        Property* property = properties[i].get();
                        if(property == nullptr)
                        {
                            continue;
                        }

                        ImGui::PushID(static_cast<int>(i));
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(property->get_label().c_str());
                        if(ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("%s", property->get_name().c_str());
                        }
                        ImGui::TableSetColumnIndex(1);
                        property->accept(property_renderer);
                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
            }

            if(ImGui::IsItemClicked())
            {
                g_selected_object = inspected;
            }
        }
    }

    ImGui::End();
}

Object* imgui_get_selected_object() noexcept
{
    return g_selected_object;
}

void imgui_set_selected_object(Object* object) noexcept
{
    g_selected_object = object;
}

void imgui_clear_selected_object() noexcept
{
    g_selected_object = nullptr;
}

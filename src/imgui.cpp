/**
 * Software Rasterizer Playground.
 *
 * ImGui support.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <format>
#include <limits>
#include <print>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "reflection/builtin_properties.h"
#include "reflection/class_registry.h"
#include "scene/properties.h"
#include "scene/scene.h"
#include "imgui.h"
#include "renderdevice.h"
#include "renderer.h"
#include "utils.h"
#include "viewport.h"

namespace
{

Object* g_selected_object = nullptr;                     // FIXME should not be global.
const reflect::ClassInfo* g_selected_class = nullptr;    // FIXME should not be global.

void validate_selected_object(Scene& scene) noexcept
{
    if(g_selected_object == nullptr)
    {
        return;
    }

    const auto& objects = scene.get_objects();
    const bool found = std::ranges::any_of(
      objects,
      [](const std::unique_ptr<Object>& object)
      {
          return object.get() == g_selected_object;
      });
    if(!found)
    {
        g_selected_object = nullptr;
    }
}

void validate_selected_class() noexcept
{
    if(g_selected_class == nullptr)
    {
        return;
    }

    const auto classes = reflect::ReflectionSystem::get_registered_classes();
    const auto it = std::ranges::find(
      classes,
      g_selected_class);
    if(it == classes.end())
    {
        g_selected_class = nullptr;
    }
}

bool class_matches_filter(
  const reflect::ClassInfo* cls,
  std::string_view filter)
{
    if(cls == nullptr)
    {
        return false;
    }

    if(filter.empty())
    {
        return true;
    }

    const std::string needle = to_lower_copy(std::string{filter});

    return to_lower_copy(cls->module_name).contains(needle)
           || to_lower_copy(cls->name).contains(needle)
           || to_lower_copy(cls->qualified_name).contains(needle);
}

const char* property_flags_badge(const reflect::PropertyFlags flags) noexcept
{
    if((flags & reflect::PropertyFlags::ReadOnly) != reflect::PropertyFlags::None)
    {
        return "RO";
    }
    return "-";
}

std::string descriptor_constraint_summary(const reflect::PropertyDescriptor& descriptor)
{
    if(descriptor.constraint == nullptr)
    {
        return "<none>";
    }

    const auto* base = descriptor.constraint.get();
    if(base->get_type_tag() == reflect::detail::type_tag<reflect::RangeConstraint<int>>())
    {
        const auto* r = static_cast<const reflect::RangeConstraint<int>*>(base);
        return std::format(
          "range<int> min={} max={} step={} clamp={}",
          r->min.has_value() ? std::format("{}", *r->min) : std::string{"-"},
          r->max.has_value() ? std::format("{}", *r->max) : std::string{"-"},
          r->step.has_value() ? std::format("{}", *r->step) : std::string{"-"},
          r->clamp ? "true" : "false");
    }
    if(base->get_type_tag() == reflect::detail::type_tag<reflect::RangeConstraint<unsigned int>>())
    {
        const auto* r = static_cast<const reflect::RangeConstraint<unsigned int>*>(base);
        return std::format(
          "range<uint> min={} max={} step={} clamp={}",
          r->min.has_value() ? std::format("{}", *r->min) : std::string{"-"},
          r->max.has_value() ? std::format("{}", *r->max) : std::string{"-"},
          r->step.has_value() ? std::format("{}", *r->step) : std::string{"-"},
          r->clamp ? "true" : "false");
    }
    if(base->get_type_tag() == reflect::detail::type_tag<reflect::RangeConstraint<float>>())
    {
        const auto* r = static_cast<const reflect::RangeConstraint<float>*>(base);
        return std::format(
          "range<float> min={} max={} step={} clamp={}",
          r->min.has_value() ? std::format("{:.3f}", *r->min) : std::string{"-"},
          r->max.has_value() ? std::format("{:.3f}", *r->max) : std::string{"-"},
          r->step.has_value() ? std::format("{:.3f}", *r->step) : std::string{"-"},
          r->clamp ? "true" : "false");
    }

    return "<custom>";
}

std::string descriptor_default_summary(const reflect::PropertyDescriptor& descriptor)
{
    if(descriptor.get_default_value() == nullptr)
    {
        return "<none>";
    }
    if(const auto* d = descriptor.try_get_default<int>())
    {
        return std::format("int {}", d->value);
    }
    if(const auto* d = descriptor.try_get_default<unsigned int>())
    {
        return std::format("uint {}", d->value);
    }
    if(const auto* d = descriptor.try_get_default<float>())
    {
        return std::format("float {:.3f}", d->value);
    }
    if(const auto* d = descriptor.try_get_default<bool>())
    {
        return std::format("bool {}", d->value ? "true" : "false");
    }
    if(const auto* d = descriptor.try_get_default<std::string>())
    {
        return std::format("string \"{}\"", d->value);
    }
    if(const auto* d = descriptor.try_get_default<ml::vec4>())
    {
        return std::format(
          "vec4 [{:.3f} {:.3f} {:.3f} {:.3f}]",
          d->value.x,
          d->value.y,
          d->value.z,
          d->value.w);
    }
    if(const auto* d = descriptor.try_get_default<ml::mat4x4>())
    {
        return std::format(
          "mat4 diag [{:.3f} {:.3f} {:.3f} {:.3f}]",
          d->value.rows[0].x,
          d->value.rows[1].y,
          d->value.rows[2].z,
          d->value.rows[3].w);
    }
    return "<custom>";
}

using ClassChildrenMap = std::unordered_map<
  const reflect::ClassInfo*,
  std::vector<const reflect::ClassInfo*>>;

bool class_tree_contains_match(
  const reflect::ClassInfo* cls,
  const ClassChildrenMap& children_by_parent,
  std::string_view filter,
  std::unordered_map<const reflect::ClassInfo*, bool>& memo)
{
    if(cls == nullptr)
    {
        return false;
    }

    const auto memo_it = memo.find(cls);
    if(memo_it != memo.end())
    {
        return memo_it->second;
    }

    bool visible = class_matches_filter(cls, filter);
    const auto children_it = children_by_parent.find(cls);
    if(children_it != children_by_parent.end())
    {
        for(const auto* child: children_it->second)
        {
            visible = visible || class_tree_contains_match(child, children_by_parent, filter, memo);
        }
    }

    memo.emplace(cls, visible);
    return visible;
}

void draw_class_tree_node(
  const reflect::ClassInfo* cls,
  const ClassChildrenMap& children_by_parent,
  std::string_view filter,
  std::unordered_map<const reflect::ClassInfo*, bool>& visible_memo)
{
    if(cls == nullptr)
    {
        return;
    }
    if(!class_tree_contains_match(cls, children_by_parent, filter, visible_memo))
    {
        return;
    }

    const auto children_it = children_by_parent.find(cls);
    const bool has_children = children_it != children_by_parent.end() && !children_it->second.empty();

    ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_OpenOnArrow
      | ImGuiTreeNodeFlags_SpanAvailWidth;
    if(!has_children)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if(cls == g_selected_class)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(cls);
    const bool open = ImGui::TreeNodeEx(cls->qualified_name.c_str(), flags);
    if(ImGui::IsItemClicked())
    {
        g_selected_class = cls;
    }

    if(open)
    {
        if(has_children)
        {
            for(const auto* child: children_it->second)
            {
                draw_class_tree_node(
                  child,
                  children_by_parent,
                  filter,
                  visible_memo);
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
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
      0.32f,
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
    ImGui::DockBuilderDockWindow("Scene Inspector", dock_left);
    ImGui::DockBuilderDockWindow("Class Inspector", dock_left);

    ImGui::DockBuilderFinish(dockspace_id);
}

class ImGuiPropertyRenderer : public reflect::PropertyVisitor
{
public:
    void visit(reflect::Property& property) override
    {
        if(auto* p = property.try_as<reflect::IntProperty>())
        {
            render_int(*p);
        }
        else if(auto* p = property.try_as<reflect::UIntProperty>())
        {
            render_uint(*p);
        }
        else if(auto* p = property.try_as<reflect::FloatProperty>())
        {
            render_float(*p);
        }
        else if(auto* p = property.try_as<reflect::BoolProperty>())
        {
            render_bool(*p);
        }
        else if(auto* p = property.try_as<reflect::StringProperty>())
        {
            render_string(*p);
        }
        else if(auto* p = property.try_as<reflect::Mat4Property>())
        {
            render_mat4(*p);
        }
        else if(auto* p = property.try_as<reflect::Vec4Property>())
        {
            render_vec4(*p);
        }
        else
        {
            ImGui::TextUnformatted("<unsupported property type>");
        }
    }

private:
    static void render_int(reflect::IntProperty& property)
    {
        if(property.is_read_only())
        {
            ImGui::Text("%d", property.get_value());
            return;
        }

        int value = property.get_value();
        const auto* range = property.try_get_range_constraint<int>();
        int min_value = 0;
        int max_value = 0;
        const int* min_ptr = nullptr;
        const int* max_ptr = nullptr;
        if(range != nullptr)
        {
            if(range->min.has_value())
            {
                min_value = *range->min;
                min_ptr = &min_value;
            }
            if(range->max.has_value())
            {
                max_value = *range->max;
                max_ptr = &max_value;
            }
        }
        const float speed = (range != nullptr && range->step.has_value())
                              ? static_cast<float>(*range->step)
                              : property.get_speed();
        const int drag_min = (min_ptr != nullptr) ? *min_ptr : std::numeric_limits<int>::min();
        const int drag_max = (max_ptr != nullptr) ? *max_ptr : std::numeric_limits<int>::max();
        const bool changed = ImGui::DragInt(
          "##value",
          &value,
          speed,
          drag_min,
          drag_max,
          "%d",
          ImGuiSliderFlags_AlwaysClamp);
        if(changed)
        {
            property.set_value(value);
            value = property.get_value();
        }
    }

    static void render_uint(reflect::UIntProperty& property)
    {
        if(property.is_read_only())
        {
            ImGui::Text("%u", property.get_value());
            return;
        }

        unsigned int value = property.get_value();
        const auto* range = property.try_get_range_constraint<unsigned int>();
        unsigned int min_value = 0;
        unsigned int max_value = 0;
        const void* min_ptr = nullptr;
        const void* max_ptr = nullptr;
        if(range != nullptr)
        {
            if(range->min.has_value())
            {
                min_value = *range->min;
                min_ptr = &min_value;
            }
            if(range->max.has_value())
            {
                max_value = *range->max;
                max_ptr = &max_value;
            }
        }
        const bool changed = ImGui::DragScalar(
          "##value",
          ImGuiDataType_U32,
          &value,
          (range != nullptr && range->step.has_value())
            ? static_cast<float>(*range->step)
            : property.get_speed(),
          min_ptr,
          max_ptr,
          "%u",
          ImGuiSliderFlags_AlwaysClamp);
        if(changed)
        {
            property.set_value(value);
            value = property.get_value();
        }
    }

    static void render_float(reflect::FloatProperty& property)
    {
        if(property.is_read_only())
        {
            ImGui::Text(property.get_format(), property.get_value());
            return;
        }

        float value = property.get_value();
        const auto* range = property.try_get_range_constraint<float>();
        float min_value = 0.0f;
        float max_value = 0.0f;
        const float* min_ptr = nullptr;
        const float* max_ptr = nullptr;
        if(range != nullptr)
        {
            if(range->min.has_value())
            {
                min_value = *range->min;
                min_ptr = &min_value;
            }
            if(range->max.has_value())
            {
                max_value = *range->max;
                max_ptr = &max_value;
            }
        }
        const float drag_min = (min_ptr != nullptr) ? *min_ptr : -std::numeric_limits<float>::max();
        const float drag_max = (max_ptr != nullptr) ? *max_ptr : std::numeric_limits<float>::max();
        const bool changed = ImGui::DragFloat(
          "##value",
          &value,
          (range != nullptr && range->step.has_value())
            ? *range->step
            : property.get_speed(),
          drag_min,
          drag_max,
          property.get_format(),
          ImGuiSliderFlags_AlwaysClamp);
        if(changed || ImGui::IsItemDeactivatedAfterEdit())
        {
            property.set_value(value);
            value = property.get_value();
        }
    }

    static void render_bool(reflect::BoolProperty& property)
    {
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

    static void render_string(reflect::StringProperty& property)
    {
        if(property.is_read_only())
        {
            ImGui::TextUnformatted(property.get_value().c_str());
            return;
        }

        std::string value = property.get_value();
        if(value.size() > property.get_max_length())
        {
            value.resize(property.get_max_length());
        }

        if(ImGui::InputText("##value", &value))
        {
            if(value.size() > property.get_max_length())
            {
                value.resize(property.get_max_length());
            }
            property.set_value(value);
        }
    }

    static void render_mat4(reflect::Mat4Property& property)
    {
        if(ImGui::SmallButton("Edit..."))
        {
            ImGui::OpenPopup("Mat4Editor");
        }

        if(ImGui::BeginPopup("Mat4Editor"))
        {
            static ml::mat4x4 edit_value = ml::mat4x4::identity();
            if(ImGui::IsWindowAppearing())
            {
                edit_value = property.get_value();
            }

            for(int row = 0; row < 4; ++row)
            {
                float row_values[4] = {
                  edit_value.rows[row].x,
                  edit_value.rows[row].y,
                  edit_value.rows[row].z,
                  edit_value.rows[row].w};
                ImGui::PushID(row);
                ImGui::PushItemWidth(260.0f);
                const bool row_changed = ImGui::DragFloat4(
                  "##row",
                  row_values,
                  0.01f,
                  0.0f,
                  0.0f,
                  "%.3f");
                ImGui::PopItemWidth();
                ImGui::PopID();
                if(row_changed)
                {
                    edit_value.rows[row].x = row_values[0];
                    edit_value.rows[row].y = row_values[1];
                    edit_value.rows[row].z = row_values[2];
                    edit_value.rows[row].w = row_values[3];
                }
            }

            if(ImGui::Button("Apply"))
            {
                property.set_value(edit_value);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if(ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    static void render_vec4(reflect::Vec4Property& property)
    {
        ml::vec4 value = property.get_value();
        float components[4] = {value.x, value.y, value.z, value.w};

        if(property.is_read_only())
        {
            ImGui::Text(
              "[%.3f %.3f %.3f %.3f]",
              components[0],
              components[1],
              components[2],
              components[3]);
            return;
        }

        if(ImGui::DragFloat4("##value", components, 0.01f, 0.0f, 0.0f, "%.3f"))
        {
            property.set_value(
              ml::vec4{
                components[0],
                components[1],
                components[2],
                components[3]});
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

void imgui_draw_scene_inspector_panel(Scene& scene)
{
    ImGui::Begin("Scene Inspector");
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
            const auto* class_info = object->get_class();
            const std::string type_name = class_info != nullptr
                                            ? std::string{class_info->name}
                                            : std::string{"Unknown"};
            const std::string object_header = std::format(
              "{} ({}.{})##{}",
              object->get_name(),
              class_info->module_name,
              type_name,
              object->get_object_id().value);

            ImGuiTreeNodeFlags header_flags =
              ImGuiTreeNodeFlags_DefaultOpen
              | ImGuiTreeNodeFlags_SpanAvailWidth;
            if(object.get() == g_selected_object)
            {
                header_flags |= ImGuiTreeNodeFlags_Selected;
            }

            if(ImGui::CollapsingHeader(object_header.c_str(), header_flags))
            {
                const std::string table_id = std::format(
                  "ObjectProperties##{}",
                  object->get_object_id().value);
                const ImGuiTableFlags table_flags =
                  ImGuiTableFlags_BordersInnerV
                  | ImGuiTableFlags_BordersOuter
                  | ImGuiTableFlags_RowBg
                  | ImGuiTableFlags_SizingFixedFit;

                if(ImGui::BeginTable(table_id.c_str(), 3, table_flags))
                {
                    ImGui::TableSetupColumn(
                      "Property",
                      ImGuiTableColumnFlags_WidthFixed,
                      80.0f);
                    ImGui::TableSetupColumn(
                      "Value",
                      ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn(
                      "Action",
                      ImGuiTableColumnFlags_WidthFixed,
                      56.0f);
                    ImGui::TableHeadersRow();

                    auto& properties = object->get_properties();
                    ImGuiPropertyRenderer property_renderer;
                    for(std::size_t i = 0; i < properties.size(); ++i)
                    {
                        auto* property = properties[i].get();
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
                        const bool can_reset =
                          !property->is_read_only()
                          && object->has_property_snapshot(property->get_name());
                        ImGui::TableSetColumnIndex(1);
                        property->accept(property_renderer);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::BeginDisabled(!can_reset);
                        if(ImGui::SmallButton("Reset"))
                        {
                            object->reset_property_to_snapshot(property->get_name());
                        }
                        ImGui::EndDisabled();
                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
            }

            if(ImGui::IsItemClicked())
            {
                g_selected_object = object.get();
            }
        }
    }

    ImGui::End();
}

void imgui_draw_class_inspector_panel()
{
    ImGui::Begin("Class Inspector");
    validate_selected_class();

    static std::string filter_text;
    std::string filter = to_lower_copy(filter_text);

    const auto classes = reflect::ReflectionSystem::get_registered_classes();
    if(classes.empty())
    {
        ImGui::TextUnformatted("No reflected classes registered.");
        ImGui::End();
        return;
    }

    std::unordered_set<const reflect::ClassInfo*> known_classes{
      classes.begin(),
      classes.end()};
    ClassChildrenMap children_by_parent;
    std::vector<const reflect::ClassInfo*> roots;
    roots.reserve(classes.size());

    for(const auto* cls: classes)
    {
        if(cls == nullptr)
        {
            continue;
        }

        const auto* parent = cls->get_super();
        const bool has_known_parent =
          parent != nullptr
          && known_classes.contains(parent);
        if(has_known_parent)
        {
            children_by_parent[parent].push_back(cls);
        }
        else
        {
            roots.push_back(cls);
        }
    }

    for(auto& [_, children]: children_by_parent)
    {
        std::ranges::sort(
          children,
          [](const reflect::ClassInfo* a, const reflect::ClassInfo* b)
          {
              if(a->qualified_name != b->qualified_name)
              {
                  return a->qualified_name < b->qualified_name;
              }
              return a->root_tag < b->root_tag;
          });
    }
    std::ranges::sort(
      roots,
      [](const reflect::ClassInfo* a, const reflect::ClassInfo* b)
      {
          if(a->qualified_name != b->qualified_name)
          {
              return a->qualified_name < b->qualified_name;
          }
          return a->root_tag < b->root_tag;
      });

    if(g_selected_class == nullptr)
    {
        g_selected_class = roots.front();
    }

    ImGui::InputTextWithHint("##class-filter", "Filter classes...", &filter_text);
    ImGui::SeparatorText("Hierarchy");

    const float avail_h = ImGui::GetContentRegionAvail().y;
    const float hierarchy_h = std::max(150.0f, avail_h * 0.42f);

    ImGui::BeginChild("ClassHierarchy", ImVec2{0, hierarchy_h}, true);
    std::unordered_map<const reflect::ClassInfo*, bool> visible_memo;
    bool any_visible = false;
    for(const auto* root: roots)
    {
        if(class_tree_contains_match(root, children_by_parent, filter, visible_memo))
        {
            any_visible = true;
            draw_class_tree_node(
              root,
              children_by_parent,
              filter,
              visible_memo);
        }
    }
    if(!any_visible)
    {
        ImGui::TextDisabled("No classes match filter.");
    }
    ImGui::EndChild();

    ImGui::SeparatorText("Details");
    ImGui::BeginChild("ClassDetails", ImVec2{0, 0}, true);

    if(g_selected_class == nullptr)
    {
        ImGui::TextUnformatted("Select a class from the hierarchy.");
    }
    else
    {
        const auto* cls = g_selected_class;
        const auto child_count_it = children_by_parent.find(cls);
        const std::size_t child_count =
          child_count_it != children_by_parent.end()
            ? child_count_it->second.size()
            : 0;

        ImGui::Text("%s", cls->qualified_name.c_str());
        ImGui::Separator();
        ImGui::Text("Module: %s", cls->module_name.c_str());
        ImGui::Text("Name: %s", cls->name.c_str());
        ImGui::Text("Size: %zu bytes", cls->size);
        ImGui::Text(
          "Parent: %s",
          cls->get_super() != nullptr
            ? cls->get_super()->qualified_name.c_str()
            : "<none>");
        ImGui::Text("Children: %zu", child_count);
        ImGui::Text("Root Tag: %p", cls->root_tag);

        ImGui::SeparatorText("Properties");
        const float properties_area_h = ImGui::GetContentRegionAvail().y;

        std::vector<const reflect::ClassInfo*> class_chain;
        for(const auto* p = cls; p != nullptr; p = p->get_super())
        {
            class_chain.push_back(p);
        }

        std::unordered_map<const reflect::PropertyDescriptor*, std::array<std::size_t, 3>>
          layout_by_descriptor;
        if(cls->factory != nullptr && cls->destroy != nullptr)
        {
            void* instance = cls->factory();
            if(instance != nullptr)
            {
                for(const auto* origin: class_chain | std::views::reverse)
                {
                    for(auto* descriptor = origin->first_property.get();
                        descriptor != nullptr;
                        descriptor = descriptor->next.get())
                    {
                        if(descriptor->construct == nullptr)
                        {
                            continue;
                        }

                        try
                        {
                            auto property = descriptor->construct(
                              instance,
                              descriptor->name,
                              descriptor->label,
                              descriptor->flags,
                              descriptor->constraint);
                            if(property == nullptr)
                            {
                                continue;
                            }

                            layout_by_descriptor.emplace(
                              descriptor,
                              std::array{
                                property->get_size(),
                                property->get_offset(),
                                property->get_alignment()});
                        }
                        catch(...)
                        {
                            // Metadata probe is best-effort in the inspector.
                        }
                    }
                }
                cls->destroy(instance);
            }
        }

        std::vector<std::pair<const reflect::PropertyDescriptor*, const reflect::ClassInfo*>> property_rows;
        for(const auto* origin: class_chain | std::views::reverse)
        {
            for(auto* descriptor = origin->first_property.get();
                descriptor != nullptr;
                descriptor = descriptor->next.get())
            {
                property_rows.emplace_back(descriptor, origin);
            }
        }

        static const reflect::PropertyDescriptor* selected_property = nullptr;
        static const reflect::ClassInfo* selected_property_origin = nullptr;
        const auto selected_it = std::ranges::find_if(
          property_rows,
          [&](const auto& row)
          {
              return row.first == selected_property;
          });
        if(selected_it == property_rows.end())
        {
            selected_property = property_rows.empty() ? nullptr : property_rows.front().first;
            selected_property_origin = property_rows.empty() ? nullptr : property_rows.front().second;
        }

        const float line_h = ImGui::GetTextLineHeightWithSpacing();
        const float property_table_min_h = line_h * 7.0f;  // Header + ~5 rows with padding.
        const float detail_panel_min_h = line_h * 7.5f;    // ~6 detail entries with padding.
        const float split_gap_h = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        const float usable_h = std::max(0.0f, properties_area_h - split_gap_h);
        const float property_table_max_h =
          std::max(property_table_min_h, usable_h - detail_panel_min_h);
        const float property_table_h = std::clamp(
          usable_h * 0.54f,
          property_table_min_h,
          property_table_max_h);
        ImGui::BeginChild("ClassPropertiesTableArea", ImVec2{0, property_table_h}, true);

        const ImGuiTableFlags property_table_flags =
          ImGuiTableFlags_BordersInnerV
          | ImGuiTableFlags_BordersOuter
          | ImGuiTableFlags_RowBg
          | ImGuiTableFlags_Resizable
          | ImGuiTableFlags_SizingFixedFit
          | ImGuiTableFlags_ScrollY
          | ImGuiTableFlags_ScrollX;
        if(ImGui::BeginTable(
             "ClassProperties",
             4,
             property_table_flags,
             ImVec2{-FLT_MIN, -FLT_MIN}))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 24.0f);
            ImGui::TableSetupColumn("Origin", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableHeadersRow();

            for(const auto& [descriptor, origin]: property_rows)
            {
                ImGui::PushID(descriptor);
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                const bool is_selected = selected_property == descriptor;
                if(ImGui::Selectable(
                     descriptor->label.c_str(),
                     is_selected,
                     ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
                {
                    selected_property = descriptor;
                    selected_property_origin = origin;
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(descriptor->name.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(property_flags_badge(descriptor->flags));

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(origin->qualified_name.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Selected Property");
        ImGui::BeginChild("ClassPropertyDetailsArea", ImVec2{0, 0}, true);
        if(selected_property == nullptr || selected_property_origin == nullptr)
        {
            ImGui::TextDisabled("Select a property to inspect metadata.");
        }
        else
        {
            const auto layout_it = layout_by_descriptor.find(selected_property);
            const bool has_layout = layout_it != layout_by_descriptor.end();

            if(ImGui::BeginTable(
                 "SelectedPropertyList",
                 2,
                 ImGuiTableFlags_BordersInnerV
                 | ImGuiTableFlags_BordersOuter
                 | ImGuiTableFlags_RowBg
                 | ImGuiTableFlags_SizingStretchProp
                 | ImGuiTableFlags_ScrollY,
                 ImVec2{-FLT_MIN, -FLT_MIN}))
            {
                ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 96.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                auto draw_detail_row = [](const char* key, const std::string& value)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(key);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushTextWrapPos(0.0f);
                    ImGui::TextUnformatted(value.c_str());
                    ImGui::PopTextWrapPos();
                };

                draw_detail_row("Label", selected_property->label);
                draw_detail_row("Name", selected_property->name);
                draw_detail_row("Origin", selected_property_origin->qualified_name);
                draw_detail_row("Flags", property_flags_badge(selected_property->flags));
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("Layout");
                ImGui::TableSetColumnIndex(1);
                if(ImGui::BeginTable(
                     "SelectedPropertyLayout",
                     2,
                     ImGuiTableFlags_BordersInnerV
                     | ImGuiTableFlags_BordersOuter
                     | ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 68.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Size");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(has_layout ? std::format("{}", layout_it->second[0]).c_str() : "n/a");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Offset");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(has_layout ? std::format("{}", layout_it->second[1]).c_str() : "n/a");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Align");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(has_layout ? std::format("{}", layout_it->second[2]).c_str() : "n/a");
                    ImGui::EndTable();
                }
                draw_detail_row("Default", descriptor_default_summary(*selected_property));
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("Constraint");
                ImGui::TableSetColumnIndex(1);
                if(ImGui::BeginTable(
                     "SelectedPropertyConstraint",
                     2,
                     ImGuiTableFlags_BordersInnerV
                     | ImGuiTableFlags_BordersOuter
                     | ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 68.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    const auto draw_constraint_row = [](const char* key, const std::string& value)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(key);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(value.c_str());
                    };

                    if(selected_property->constraint == nullptr)
                    {
                        draw_constraint_row("Type", "<none>");
                    }
                    else
                    {
                        const auto* base = selected_property->constraint.get();
                        if(base->get_type_tag() == reflect::detail::type_tag<reflect::RangeConstraint<int>>())
                        {
                            const auto* r = static_cast<const reflect::RangeConstraint<int>*>(base);
                            draw_constraint_row("Type", "range<int>");
                            draw_constraint_row("Min", r->min.has_value() ? std::format("{}", *r->min) : "-");
                            draw_constraint_row("Max", r->max.has_value() ? std::format("{}", *r->max) : "-");
                            draw_constraint_row("Step", r->step.has_value() ? std::format("{}", *r->step) : "-");
                            draw_constraint_row("Clamp", r->clamp ? "true" : "false");
                        }
                        else if(base->get_type_tag() == reflect::detail::type_tag<reflect::RangeConstraint<unsigned int>>())
                        {
                            const auto* r = static_cast<const reflect::RangeConstraint<unsigned int>*>(base);
                            draw_constraint_row("Type", "range<uint>");
                            draw_constraint_row("Min", r->min.has_value() ? std::format("{}", *r->min) : "-");
                            draw_constraint_row("Max", r->max.has_value() ? std::format("{}", *r->max) : "-");
                            draw_constraint_row("Step", r->step.has_value() ? std::format("{}", *r->step) : "-");
                            draw_constraint_row("Clamp", r->clamp ? "true" : "false");
                        }
                        else if(base->get_type_tag() == reflect::detail::type_tag<reflect::RangeConstraint<float>>())
                        {
                            const auto* r = static_cast<const reflect::RangeConstraint<float>*>(base);
                            draw_constraint_row("Type", "range<float>");
                            draw_constraint_row("Min", r->min.has_value() ? std::format("{:.3f}", *r->min) : "-");
                            draw_constraint_row("Max", r->max.has_value() ? std::format("{:.3f}", *r->max) : "-");
                            draw_constraint_row("Step", r->step.has_value() ? std::format("{:.3f}", *r->step) : "-");
                            draw_constraint_row("Clamp", r->clamp ? "true" : "false");
                        }
                        else
                        {
                            draw_constraint_row("Type", "<custom>");
                            draw_constraint_row("Value", descriptor_constraint_summary(*selected_property));
                        }
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }

    ImGui::EndChild();

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

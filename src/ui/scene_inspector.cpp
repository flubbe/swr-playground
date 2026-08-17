/**
 * Software Rasterizer Playground.
 *
 * Class inspector panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <format>
#include <string>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "containers/format.h"
#include "reflection/builtin_properties.h"
#include "renderer/render_device.h"
#include "scene/properties.h"
#include "scene/scene.h"
#include "scene/static_mesh.h"
#include "ui/imgui.h"

namespace
{

void validate_selected_object(
  imgui::State& ui_state,
  Scene& scene) noexcept
{
    if(ui_state.selected_scene_object == nullptr)
    {
        return;
    }

    const auto& objects = scene.get_objects();
    const bool found = std::ranges::any_of(
      objects,
      [&](const auto& object)
      {
          return object.get() == ui_state.selected_scene_object;
      });
    if(!found)
    {
        ui_state.selected_scene_object = nullptr;
    }
}

class ImGuiPropertyRenderer : public reflect::PropertyVisitor
{
    Object& object;

public:
    explicit ImGuiPropertyRenderer(Object& object)
    : object{object}
    {
    }

    void visit(reflect::Property& property) override
    {
        if(auto* p = property.try_as<reflect::IntProperty>())
        {
            render_int(*p, object);
        }
        else if(auto* p = property.try_as<reflect::UIntProperty>())
        {
            render_uint(*p, object);
        }
        else if(auto* p = property.try_as<reflect::FloatProperty>())
        {
            render_float(*p, object);
        }
        else if(auto* p = property.try_as<reflect::BoolProperty>())
        {
            render_bool(*p, object);
        }
        else if(auto* p = property.try_as<reflect::StringProperty>())
        {
            render_string(*p, object);
        }
        else if(auto* p = property.try_as<reflect::PathProperty>())
        {
            render_path(*p, object);
        }
#if SWR_CUSTOM_STRING_TYPE
        else if(auto* p = property.try_as<reflect::SwrStringProperty>())
        {
            render_string(*p, object);
        }
#endif /* SWR_CUSTOM_STRING_TYPE */
        else if(auto* p = property.try_as<reflect::Mat4Property>())
        {
            render_mat4(*p, object);
        }
        else if(auto* p = property.try_as<reflect::Vec4Property>())
        {
            render_vec4(*p, object);
        }
        else
        {
            ImGui::TextUnformatted("<unsupported property type>");
        }
    }

private:
    static void render_int(reflect::IntProperty& property, Object& object)
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
        const float speed =
          (range != nullptr && range->step.has_value())
            ? static_cast<float>(*range->step)
            : property.get_speed();
        const int drag_min =
          (min_ptr != nullptr)
            ? *min_ptr
            : std::numeric_limits<int>::min();
        const int drag_max =
          (max_ptr != nullptr)
            ? *max_ptr
            : std::numeric_limits<int>::max();
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
            if(property.set_value(value))
            {
                object.on_properties_changed();
            }
            value = property.get_value();
        }
    }

    static void render_uint(reflect::UIntProperty& property, Object& object)
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
            if(property.set_value(value))
            {
                object.on_properties_changed();
            }
            value = property.get_value();
        }
    }

    static void render_float(reflect::FloatProperty& property, Object& object)
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
        const float drag_min =
          (min_ptr != nullptr)
            ? *min_ptr
            : -std::numeric_limits<float>::max();
        const float drag_max =
          (max_ptr != nullptr)
            ? *max_ptr
            : std::numeric_limits<float>::max();
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
            if(property.set_value(value))
            {
                object.on_properties_changed();
            }
            value = property.get_value();
        }
    }

    static void render_bool(reflect::BoolProperty& property, Object& object)
    {
        if(property.is_read_only())
        {
            ImGui::TextUnformatted(property.get_value() ? "true" : "false");
            return;
        }

        bool value = property.get_value();
        if(ImGui::Checkbox("##value", &value))
        {
            if(property.set_value(value))
            {
                object.on_properties_changed();
            }
        }
    }

    template<typename T>
        requires std::is_same_v<T, reflect::StringProperty>
#if SWR_CUSTOM_STRING_TYPE
                 || std::is_same_v<T, reflect::SwrStringProperty>
#endif /* SWR_CUSTOM_STRING_TYPE */
    static void render_string(
      T& property,
      Object& object)
    {
        if(property.is_read_only())
        {
            ImGui::TextUnformatted(property.get_value().c_str());
            return;
        }

        std::string value = swr::std_string_from(property.get_value());

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
            if(property.set_value(value))
            {
                object.on_properties_changed();
            }
        }
    }

    static void render_path(
      reflect::PathProperty& property,
      Object& object)
    {
        if(property.is_read_only())
        {
            ImGui::TextUnformatted(property.get_value().c_str());
            return;
        }

        std::string value = swr::std_string_from(property.get_value().string());
        if(ImGui::InputText("##value", &value))
        {
            if(property.set_value(value))
            {
                object.on_properties_changed();
            }
        }
    }

    static void render_mat4(reflect::Mat4Property& property, Object& object)
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

            bool changed = false;

            for(int row = 0; row < 4; ++row)
            {
                float row_values[4] = {
                  edit_value.rows[row].x,
                  edit_value.rows[row].y,
                  edit_value.rows[row].z,
                  edit_value.rows[row].w};

                ImGui::PushID(row);
                ImGui::PushItemWidth(260.0f);

                if(ImGui::DragFloat4("##row", row_values, 0.01f, 0.0f, 0.0f, "%.3f"))
                {
                    edit_value.rows[row].x = row_values[0];
                    edit_value.rows[row].y = row_values[1];
                    edit_value.rows[row].z = row_values[2];
                    edit_value.rows[row].w = row_values[3];
                    changed = true;
                }

                ImGui::PopItemWidth();
                ImGui::PopID();
            }

            if(changed)
            {
                if(property.set_value(edit_value))
                {
                    object.on_properties_changed();
                }
            }

            ImGui::EndPopup();
        }
    }

    static void render_vec4(reflect::Vec4Property& property, Object& object)
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
            if(property.set_value(
                 ml::vec4{
                   components[0],
                   components[1],
                   components[2],
                   components[3]}))
            {
                object.on_properties_changed();
            }
        }
    }
};

void draw_static_mesh_sections(
  const StaticMesh& mesh)
{
    if(!mesh.has_mesh_sections())
    {
        return;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Mesh sections");

    const ImGuiTableFlags table_flags =
      ImGuiTableFlags_BordersInnerV
      | ImGuiTableFlags_BordersOuter
      | ImGuiTableFlags_RowBg
      | ImGuiTableFlags_SizingFixedFit;

    if(ImGui::BeginTable("MeshSections", 4, table_flags))
    {
        ImGui::TableSetupColumn("LOD", ImGuiTableColumnFlags_WidthFixed, 32.0f);
        ImGui::TableSetupColumn("Section", ImGuiTableColumnFlags_WidthFixed, 32.0f);
        ImGui::TableSetupColumn("Material", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for(std::size_t lod_index = 0; lod_index < mesh.get_lod_count(); ++lod_index)
        {
            const auto& lod = mesh.get_lod(lod_index);
            for(std::size_t section_index = 0; section_index < lod.mesh_sections.size(); ++section_index)
            {
                const auto& section = lod.mesh_sections[section_index];
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%zu", lod_index);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", section_index);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", section.material.get_path().c_str());
            }
        }

        ImGui::EndTable();
    }
}

}    // namespace

namespace imgui
{

void draw_scene_inspector_panel(
  State& ui_state,
  Scene& scene)
{
    ImGui::Begin("Scene Inspector");
    validate_selected_object(ui_state, scene);

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
            const swr::string type_name =
              class_info != nullptr
                ? swr::string{class_info->name}
                : swr::string{"Unknown"};

            static swr::string object_header;
            object_header.clear();

            std::format_to(
              std::back_inserter(object_header),
              "{} ({}.{})##{}",
              object->get_name(),
              class_info->module_name,
              type_name,
              object->get_object_id().value);

            ImGuiTreeNodeFlags header_flags =
              ImGuiTreeNodeFlags_DefaultOpen
              | ImGuiTreeNodeFlags_SpanAvailWidth;
            if(object.get() == ui_state.selected_scene_object)
            {
                header_flags |= ImGuiTreeNodeFlags_Selected;
            }

            if(ImGui::CollapsingHeader(object_header.c_str(), header_flags))
            {
                static swr::string table_id;
                table_id.clear();

                std::format_to(
                  std::back_inserter(table_id),
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
                    ImGuiPropertyRenderer property_renderer{*object};
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

                if(auto* mesh = reflect::try_cast<StaticMesh>(object.get()))
                {
                    draw_static_mesh_sections(*mesh);
                }
            }

            if(ImGui::IsItemClicked())
            {
                ui_state.selected_scene_object = object.get();
            }
        }
    }

    ImGui::End();
}

}    // namespace imgui

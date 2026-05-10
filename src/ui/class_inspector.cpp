/**
 * Software Rasterizer Playground.
 *
 * class inspector panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "ml/all.h"

#include "reflection/class_registry.h"
#include "imgui.h"
#include "utils.h"

namespace
{

void validate_selected_class(
  imgui::State& ui_state) noexcept
{
    if(ui_state.selected_class == nullptr)
    {
        return;
    }

    const auto classes = reflect::ReflectionSystem::get_registered_classes();
    const auto it = std::ranges::find(
      classes,
      ui_state.selected_class);
    if(it == classes.end())
    {
        ui_state.selected_class = nullptr;
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

const char* property_flags_badge(
  const reflect::PropertyFlags flags) noexcept
{
    if((flags & reflect::PropertyFlags::ReadOnly) != reflect::PropertyFlags::None)
    {
        return "RO";
    }
    return "-";
}

std::string descriptor_constraint_summary(
  const reflect::PropertyDescriptor& descriptor)
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

std::string descriptor_default_summary(
  const reflect::PropertyDescriptor& descriptor)
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
  imgui::State& ui_state,
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
    if(cls == ui_state.selected_class)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(cls);
    const bool open = ImGui::TreeNodeEx(cls->qualified_name.c_str(), flags);
    if(ImGui::IsItemClicked())
    {
        ui_state.selected_class = cls;
    }

    if(open)
    {
        if(has_children)
        {
            for(const auto* child: children_it->second)
            {
                draw_class_tree_node(
                  ui_state,
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
}    // namespace

namespace imgui
{

struct ClassInspectorCache
{
    const reflect::ClassInfo* cls{nullptr};
    std::vector<const reflect::ClassInfo*> class_chain;
    std::unordered_map<
      const reflect::PropertyDescriptor*,
      std::array<std::size_t, 3>>
      layout_by_descriptor;
    std::vector<
      std::pair<
        const reflect::PropertyDescriptor*,
        const reflect::ClassInfo*>>
      property_rows;
};

void draw_class_inspector_panel(
  State& ui_state)
{
    ImGui::Begin("Class Inspector");
    validate_selected_class(ui_state);

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

    if(ui_state.selected_class == nullptr)
    {
        ui_state.selected_class = roots.front();
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
              ui_state,
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

    if(ui_state.selected_class == nullptr)
    {
        ImGui::TextUnformatted("Select a class from the hierarchy.");
    }
    else
    {
        const auto* cls = ui_state.selected_class;
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
        static ClassInspectorCache cache;

        if(cache.cls != cls)
        {
            cache.cls = cls;
            cache.class_chain.clear();
            cache.layout_by_descriptor.clear();
            cache.property_rows.clear();

            for(const auto* p = cls; p != nullptr; p = p->get_super())
            {
                cache.class_chain.push_back(p);
            }

            for(const auto* origin: cache.class_chain | std::views::reverse)
            {
                for(auto* descriptor = origin->first_property.get();
                    descriptor != nullptr;
                    descriptor = descriptor->next.get())
                {
                    cache.property_rows.emplace_back(descriptor, origin);
                }
            }

            if(cls->factory != nullptr && cls->destroy != nullptr)
            {
                void* instance = cls->factory();
                if(instance != nullptr)
                {
                    for(const auto* origin: cache.class_chain | std::views::reverse)
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

                                cache.layout_by_descriptor.emplace(
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
        }

        const auto& layout_by_descriptor = cache.layout_by_descriptor;
        const auto& property_rows = cache.property_rows;

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

        const ImGuiTableFlags property_table_flags =
          ImGuiTableFlags_BordersInnerV
          | ImGuiTableFlags_BordersOuter
          | ImGuiTableFlags_RowBg
          | ImGuiTableFlags_Resizable
          | ImGuiTableFlags_SizingFixedFit
          | ImGuiTableFlags_ScrollX;
        if(ImGui::BeginTable(
             "ClassProperties",
             4,
             property_table_flags))
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

        ImGui::SeparatorText("Selected Property");
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
                   | ImGuiTableFlags_SizingStretchProp))
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
                    ImGui::TextUnformatted("Align");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(has_layout ? std::format("{}", layout_it->second[2]).c_str() : "n/a");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Offset");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(has_layout ? std::format("{}", layout_it->second[1]).c_str() : "n/a");
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
    }

    ImGui::End();
}

}    // namespace imgui

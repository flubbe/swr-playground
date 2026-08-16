/**
 * Software Rasterizer Playground.
 *
 * Console panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <string>
#include <algorithm>
#include <cfloat>

#include <imgui.h>

#include "containers/unordered_map.h"
#include "ui/imgui.h"
#include "logging.h"

namespace imgui
{

namespace
{

enum class ConsoleLineType
{
    Log,
    Warn,
    Error,
    Unknown,
};

struct SystemFilterEntry
{
    swr::string name;
    bool enabled{true};
};

ConsoleLineType to_console_line_type(logging::LogLevel level)
{
    switch(level)
    {
    case logging::LogLevel::Log:
        return ConsoleLineType::Log;
    case logging::LogLevel::Warn:
        return ConsoleLineType::Warn;
    case logging::LogLevel::Error:
        return ConsoleLineType::Error;
    default:
        return ConsoleLineType::Unknown;
    }
}

}    // namespace

void draw_console_panel(
  logging::BufferedLogDevice& log_device)
{
    static bool show_log = true;
    static bool show_warn = true;
    static bool show_error = true;
    static swr::vector<SystemFilterEntry> label_filters;

    ImGui::Begin("Console");

    static swr::vector<logging::LogRecord> records;
    log_device.get_records(records);

    swr::unordered_map<
      std::string_view,
      std::size_t,
      memory::MemoryDomain::Frame>
      label_index;
    label_index.clear();
    label_index.reserve(label_filters.size());
    for(std::size_t i = 0; i < label_filters.size(); ++i)
    {
        label_index[label_filters[i].name] = i;
    }

    for(const logging::LogRecord& record: records)
    {
        if(record.label.empty())
        {
            continue;
        }

        const auto index_it = label_index.find(record.label);
        if(index_it == label_index.end())
        {
            label_filters.push_back(SystemFilterEntry{
              .name = record.label,
              .enabled = true,
            });
            label_index[record.label] = label_filters.size() - 1;
        }
    }

    ImFont* small_font = get_small_ui_font();
    if(small_font != nullptr)
    {
        ImGui::PushFont(small_font);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{6.0f, 3.0f});

    if(ImGui::Button("Clear"))
    {
        log_device.clear();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(64.0f);
    if(ImGui::BeginCombo("##ConsoleLevelFilter", "Level"))
    {
        ImGui::Checkbox(
          logging::to_string(logging::LogLevel::Log),
          &show_log);
        ImGui::Checkbox(
          logging::to_string(logging::LogLevel::Warn),
          &show_warn);
        ImGui::Checkbox(
          logging::to_string(logging::LogLevel::Error),
          &show_error);
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(84.0f);
    if(ImGui::BeginCombo("##ConsoleLoggerFilter", "Logger"))
    {
        if(label_filters.empty())
        {
            ImGui::BeginDisabled(true);
            bool dummy = false;
            ImGui::Checkbox("<No label>", &dummy);
            ImGui::EndDisabled();
        }
        else
        {
            for(SystemFilterEntry& label_filter: label_filters)
            {
                ImGui::Checkbox(
                  label_filter.name.c_str(),
                  &label_filter.enabled);
            }
        }
        ImGui::EndCombo();
    }

    ImGui::PopStyleVar();
    if(small_font != nullptr)
    {
        ImGui::PopFont();
    }

    ImGui::Separator();

    swr::unordered_map<
      swr::string,
      bool,
      memory::MemoryDomain::Frame>
      label_enabled;
    label_enabled.clear();
    label_enabled.reserve(label_filters.size());
    for(const SystemFilterEntry& filter_entry: label_filters)
    {
        label_enabled[filter_entry.name] = filter_entry.enabled;
    }

    ImFont* mono_font = get_console_monospace_font();
    if(mono_font != nullptr)
    {
        ImGui::PushFont(mono_font);
    }

    ImGui::BeginChild(
      "##ConsoleScrollRegion",
      ImVec2{-FLT_MIN, -FLT_MIN},
      false,
      ImGuiWindowFlags_HorizontalScrollbar);

    const float scroll_y = ImGui::GetScrollY();
    const float max_scroll_y = ImGui::GetScrollMaxY();
    const bool at_bottom_before =
      max_scroll_y <= 0.f
      || scroll_y >= max_scroll_y - 1.f;

    static swr::string visible_text;
    visible_text.clear();

    static bool follow_tail = true;
    static std::size_t previous_visible_text_size = 0;

    for(const logging::LogRecord& record: records)
    {
        const ConsoleLineType type = to_console_line_type(record.level);

        bool matches_type = false;
        switch(type)
        {
        case ConsoleLineType::Log:
            matches_type = show_log;
            break;
        case ConsoleLineType::Warn:
            matches_type = show_warn;
            break;
        case ConsoleLineType::Error:
            matches_type = show_error;
            break;
        case ConsoleLineType::Unknown:
            matches_type = show_log;
            break;
        }

        bool matches_label = true;
        if(!record.label.empty())
        {
            const auto enabled_it = label_enabled.find(record.label);
            if(enabled_it != label_enabled.end())
            {
                matches_label = enabled_it->second;
            }
        }

        if(!matches_type || !matches_label)
        {
            continue;
        }

        visible_text += record.display_line;
        visible_text.push_back('\n');
    }

    static swr::vector<char> visible_buffer;
    visible_buffer.assign(visible_text.begin(), visible_text.end());
    visible_buffer.push_back('\0');

    const bool visible_text_changed =
      visible_text.size() != previous_visible_text_size;

    ImGui::TextUnformatted(
      visible_buffer.data(),
      visible_buffer.data() + visible_text.size());

    if(visible_text_changed && follow_tail)
    {
        ImGui::SetScrollHereY(1.0f);
    }

    const float scroll_y_after = ImGui::GetScrollY();
    const float max_scroll_y_after = ImGui::GetScrollMaxY();
    const bool at_bottom_after =
      max_scroll_y_after <= 0.f
      || scroll_y_after >= max_scroll_y_after - 1.f;

    if(ImGui::IsWindowHovered() && !at_bottom_after && !at_bottom_before)
    {
        follow_tail = false;
    }
    else if(at_bottom_after)
    {
        follow_tail = true;
    }

    previous_visible_text_size = visible_text.size();

    if(ImGui::BeginPopupContextWindow("##ConsoleTextPopup"))
    {
        if(ImGui::MenuItem("Copy all"))
        {
            ImGui::SetClipboardText(visible_text.c_str());
        }
        ImGui::EndPopup();
    }

    ImGui::EndChild();

    if(mono_font != nullptr)
    {
        ImGui::PopFont();
    }

    ImGui::End();
}

}    // namespace imgui

/**
 * Software Rasterizer Playground.
 *
 * console panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <string>

#include <imgui.h>

#include "logging.h"

namespace imgui
{

void draw_console_panel(
  logging::BufferedLogDevice& log_device)
{
    ImGui::Begin("Console");

    if(ImGui::Button("Clear"))
    {
        log_device.clear();
    }

    ImGui::Separator();

    ImGui::BeginChild(
      "ConsoleScrollRegion",
      ImVec2{0, 0},
      false,
      ImGuiWindowFlags_HorizontalScrollbar);
    const bool scroll_to_bottom =
      ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
    for(const std::string& line: log_device.get_lines())
    {
        ImGui::TextUnformatted(line.c_str());
    }
    if(scroll_to_bottom)
    {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

}    // namespace imgui

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
#include <vector>

#include <imgui.h>

namespace imgui
{

void draw_console_panel(
  std::vector<std::string>& log_lines)
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

}    // namespace imgui

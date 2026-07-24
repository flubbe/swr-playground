/**
 * Software Rasterizer Playground.
 *
 * Memory profiler panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <array>
#include <cstddef>

#include <imgui.h>

#include "memory/manager.h"
#include "ui/imgui.h"

namespace imgui
{

namespace
{

constexpr std::size_t history_samples = 240;

float to_megabytes(std::size_t bytes)
{
    return static_cast<float>(bytes) / (1024.f * 1024.f);
}

}    // namespace

void draw_memory_profiler_panel()
{
    static std::array<float, history_samples> live_memory_history{};
    static std::array<float, history_samples> peak_memory_history{};
    static std::array<float, history_samples> alloc_rate_history{};
    static std::size_t history_index = 0;
    static std::size_t sample_count = 0;
    static std::size_t previous_allocate_calls = 0;

    const memory::MemoryStats memory_stats = memory::stats();
    const memory::BumpAllocatorStats bump_stats = memory::frame_bump()->get_stats();

    const float live_mb = to_megabytes(memory_stats.bytes_live);
    const float peak_mb = to_megabytes(memory_stats.bytes_peak);
    const std::size_t allocate_delta =
      memory_stats.allocate_calls >= previous_allocate_calls
        ? memory_stats.allocate_calls - previous_allocate_calls
        : 0;
    previous_allocate_calls = memory_stats.allocate_calls;

    live_memory_history[history_index] = live_mb;
    peak_memory_history[history_index] = peak_mb;
    alloc_rate_history[history_index] = static_cast<float>(allocate_delta);
    history_index = (history_index + 1) % history_samples;
    sample_count = std::min(sample_count + 1, history_samples);

    ImGui::Begin("Memory");

    if(ImGui::BeginTable(
         "MemoryProfilerStats",
         2,
         ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Metric");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Live memory");
        ImGui::TableNextColumn();
        ImGui::Text("%.3f MB (%zu B)", live_mb, memory_stats.bytes_live);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Peak memory");
        ImGui::TableNextColumn();
        ImGui::Text("%.3f MB (%zu B)", peak_mb, memory_stats.bytes_peak);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Total allocated");
        ImGui::TableNextColumn();
        ImGui::Text(
          "%.3f MB (%zu B)",
          to_megabytes(memory_stats.bytes_total_allocated),
          memory_stats.bytes_total_allocated);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Allocate calls");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", memory_stats.allocate_calls);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Deallocate calls");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", memory_stats.deallocate_calls);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Active");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", memory_stats.allocate_calls - memory_stats.deallocate_calls);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Allocs/frame");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", allocate_delta);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Bump size");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", bump_stats.used_before_reset);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Bump free");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", bump_stats.free_before_reset);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Bump peak");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", bump_stats.used_peak);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Bump allocs");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", bump_stats.allocations);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Bump deallocs/frame");
        ImGui::TableNextColumn();
        ImGui::Text("%zu", bump_stats.deallocations);

        ImGui::EndTable();
    }

    ImGui::Spacing();

    const float memory_graph_max =
      std::max(peak_mb * 1.1f, 0.1f);
    ImGui::TextUnformatted("Live Memory (MB)");
    ImGui::PlotLines(
      "##LiveMemoryHistory",
      live_memory_history.data(),
      static_cast<int>(sample_count),
      static_cast<int>(history_index % std::max<std::size_t>(sample_count, 1)),
      nullptr,
      0.f,
      memory_graph_max,
      ImVec2{0.f, 80.f});

    ImGui::TextUnformatted("Peak Memory (MB)");
    ImGui::PlotLines(
      "##PeakMemoryHistory",
      peak_memory_history.data(),
      static_cast<int>(sample_count),
      static_cast<int>(history_index % std::max<std::size_t>(sample_count, 1)),
      nullptr,
      0.f,
      memory_graph_max,
      ImVec2{0.f, 80.f});

    const float alloc_graph_max = std::max(
      *std::max_element(
        alloc_rate_history.begin(),
        alloc_rate_history.end()),
      1.f);
    ImGui::TextUnformatted("Allocation Calls / Frame");
    ImGui::PlotHistogram(
      "##AllocRateHistory",
      alloc_rate_history.data(),
      static_cast<int>(sample_count),
      static_cast<int>(history_index % std::max<std::size_t>(sample_count, 1)),
      nullptr,
      0.f,
      alloc_graph_max * 1.1f,
      ImVec2{0.f, 70.f});

    ImGui::End();
}

}    // namespace imgui

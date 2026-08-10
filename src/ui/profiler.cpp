/**
 * Software Rasterizer Playground.
 *
 * Profiler panel.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>

#include <imgui.h>

#include "containers/vector.h"
#include "renderer/renderer.h"
#include "ui/imgui.h"

namespace imgui
{

namespace
{

constexpr std::size_t timing_history_samples = 300;

float percentile(
  swr::vector<float>& values,
  float p)
{
    if(values.empty())
    {
        return 0.f;
    }

    p = std::clamp(p, 0.f, 1.f);
    const std::size_t index = static_cast<std::size_t>(
      p * static_cast<float>(values.size() - 1));
    std::nth_element(
      values.begin(),
      values.begin() + static_cast<std::ptrdiff_t>(index),
      values.end());
    return values[index];
}

float average(
  const swr::vector<float>& values)
{
    if(values.empty())
    {
        return 0.f;
    }

    const float sum = std::accumulate(
      values.begin(),
      values.end(),
      0.f);
    return sum / static_cast<float>(values.size());
}

}    // namespace

void draw_profiler_panel(Renderer& renderer)
{
    static std::array<float, timing_history_samples> frame_ms_history{};
    static std::array<float, timing_history_samples> render_ms_history{};
    static std::array<float, timing_history_samples> fps_history{};
    static std::size_t history_index = 0;
    static std::size_t sample_count = 0;

    const ImGuiIO& io = ImGui::GetIO();
    const float fps = io.Framerate;
    const float frame_ms = 1000.f / std::max(fps, 0.001f);
    const float render_ms = 1000.f * renderer.get_render_time();

    frame_ms_history[history_index] = frame_ms;
    render_ms_history[history_index] = render_ms;
    fps_history[history_index] = fps;
    history_index = (history_index + 1) % timing_history_samples;
    sample_count = std::min(sample_count + 1, timing_history_samples);

    static swr::vector<float> frame_samples;
    static swr::vector<float> render_samples;
    static swr::vector<float> fps_samples;

    frame_samples.clear();
    render_samples.clear();
    fps_samples.clear();

    frame_samples.reserve(timing_history_samples);
    render_samples.reserve(timing_history_samples);
    fps_samples.reserve(timing_history_samples);

    for(std::size_t i = 0; i < sample_count; ++i)
    {
        frame_samples.push_back(frame_ms_history[i]);
        render_samples.push_back(render_ms_history[i]);
        fps_samples.push_back(fps_history[i]);
    }

    const float frame_avg = average(frame_samples);
    const float frame_min = frame_samples.empty() ? 0.f : *std::min_element(frame_samples.begin(), frame_samples.end());
    const float frame_max = frame_samples.empty() ? 0.f : *std::max_element(frame_samples.begin(), frame_samples.end());

    const float render_avg = average(render_samples);
    const float render_min = render_samples.empty() ? 0.f : *std::min_element(render_samples.begin(), render_samples.end());
    const float render_max = render_samples.empty() ? 0.f : *std::max_element(render_samples.begin(), render_samples.end());

    const float fps_avg = average(fps_samples);

    static swr::vector<float> fps_scratch;
    fps_scratch.reserve(timing_history_samples);

    fps_scratch.clear();
    std::ranges::copy(
      fps_samples.begin(),
      fps_samples.end(),
      std::back_inserter(fps_scratch));

    const float fps_p1 = percentile(fps_scratch, 0.01f);

    fps_scratch.clear();
    std::ranges::copy(
      fps_samples.begin(),
      fps_samples.end(),
      std::back_inserter(fps_scratch));

    const float fps_p99 = percentile(fps_scratch, 0.99f);

    ImGui::Begin("Profiler");

    if(ImGui::BeginTable(
         "ProfilerStats",
         2,
         ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Metric");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("FPS (current)");
        ImGui::TableNextColumn();
        ImGui::Text("%.1f", fps);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("FPS (avg)");
        ImGui::TableNextColumn();
        ImGui::Text("%.1f", fps_avg);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("FPS (P1 / P99)");
        ImGui::TableNextColumn();
        ImGui::Text("%.1f / %.1f", fps_p1, fps_p99);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Frame ms (avg/min/max)");
        ImGui::TableNextColumn();
        ImGui::Text("%.3f / %.3f / %.3f", frame_avg, frame_min, frame_max);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Render ms (avg/min/max)");
        ImGui::TableNextColumn();
        ImGui::Text("%.3f / %.3f / %.3f", render_avg, render_min, render_max);

        ImGui::EndTable();
    }

    ImGui::Spacing();

    const int offset = static_cast<int>(history_index % std::max<std::size_t>(sample_count, 1));

    ImGui::TextUnformatted("Frame Time (ms)");
    ImGui::PlotHistogram(
      "##FrameTimeHistogram",
      frame_ms_history.data(),
      static_cast<int>(sample_count),
      offset,
      nullptr,
      0.f,
      std::max(frame_max * 1.1f, 0.5f),
      ImVec2{0.f, 90.f});

    ImGui::TextUnformatted("Render Time (ms)");
    ImGui::PlotHistogram(
      "##RenderTimeHistogram",
      render_ms_history.data(),
      static_cast<int>(sample_count),
      offset,
      nullptr,
      0.f,
      std::max(render_max * 1.1f, 0.5f),
      ImVec2{0.f, 90.f});

    ImGui::TextUnformatted("FPS");
    ImGui::PlotLines(
      "##FpsHistory",
      fps_history.data(),
      static_cast<int>(sample_count),
      offset,
      nullptr,
      0.f,
      std::max(fps_p99 * 1.1f, 30.f),
      ImVec2{0.f, 90.f});

    ImGui::End();
}

}    // namespace imgui

/**
 * Software Rasterizer Playground.
 *
 * A renderer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "ml/all.h"
#include "render_types.h"
#include "shader_cache.h"

class Scene;
class Camera;
class Viewport;
struct ViewportDisplaySettings;
class RenderDevice;

struct RendererStats
{
    static constexpr std::size_t tracked_lod_count{3};

    std::size_t static_meshes{0};
    std::size_t mesh_sections_drawn{0};
    std::size_t mesh_sections_culled{0};
    std::size_t triangles_submitted{0};
    std::size_t triangles_frustum_culled{0};
    std::array<std::size_t, tracked_lod_count> static_mesh_lods_selected{};
    std::size_t static_mesh_lods_selected_overflow{0};
};

struct SortingBenchmarkResults
{
    float time_with_sorting{0.f};
    float time_without_sorting{0.f};
    std::size_t iterations{0};

    [[nodiscard]]
    float get_difference() const noexcept
    {
        return time_without_sorting - time_with_sorting;
    }

    [[nodiscard]]
    float get_percentage_improvement() const noexcept
    {
        if(time_without_sorting == 0.f)
            return 0.f;
        return (get_difference() / time_without_sorting) * 100.f;
    }
};

/** Mesh submission sorting mode. */
enum class SortMode : std::uint8_t
{
    FullSort, /** O(n log n) full sort by depth. */
    BinSort   /** O(n) coarse binning by depth. */
};

struct SortingBenchmarkState
{
    bool active{false};
    bool sorted_phase{true};

    std::size_t target_iterations{0};
    std::size_t current_iteration{0};
    float total_time_sorted{0.f};
    float total_time_unsorted{0.f};
    std::array<ml::vec3, 16> camera_positions{};
    ml::mat4x4 saved_camera_transform{ml::mat4x4::identity()};
};

struct ComparativeBenchmarkState
{
    bool active{false};
    swr::vector<SortMode> modes{};
    std::size_t current_mode_index{0};
    swr::vector<SortingBenchmarkResults> results{};
    SortMode saved_sort_mode{SortMode::FullSort};
    std::size_t iterations_per_mode{0};
};

/** Shadow map PCF filtering mode. */
enum class ShadowPcfMode : int
{
    Off = 0,                      /** No PCF filtering. */
    LegacyNearest3x3 = 1,         /** Legacy 3x3 PCF with nearest-neighbor depth compares. */
    LegacyBilinear = 2,           /** Legacy single bilinear depth compare. */
    ModernNearest3x3 = 3,         /** Shadow-sampler 3x3 PCF with nearest compare filtering. */
    ModernBilinear3x3 = 4,        /** Shadow-sampler 3x3 PCF with bilinear compare filtering. */
    Stochastic4Tap = 5,           /** Four jittered shadow-sampler taps for cheaper soft shadows. */
    Stochastic5Tap = 6,           /** Four jittered taps plus a center tap for slightly higher quality. */
    Stochastic4TapStable = 7,     /** Four jittered taps with shadow-texel-stable rotation. */
    Stochastic4TapInterleaved = 8 /** Four jittered taps with an interleaved rotation pattern. */
};

class Renderer final
{
    static constexpr int shadow_map_resolution = 1024;

    RenderDevice& device;
    ShaderCache shader_cache;

    float render_time{0.f};
    RendererStats render_stats;
    SortingBenchmarkResults benchmark_results{};
    SortingBenchmarkState benchmark_state{};
    SortMode sort_mode{SortMode::FullSort};
    std::size_t depth_bin_count{8};
    ComparativeBenchmarkState comparative_state{};
    ShadowPcfMode shadow_pcf_mode{ShadowPcfMode::Off};

    MaterialHandle shadow_material{0};

    /*
     * Viewport overlays.
     */

    MeshSection overlay_grid;
    MeshSection overlay_spotlight_depth;
    ShadowMapHandle shadow_map{};

    void create_grid_mesh();
    void release_grid_mesh();
    void create_spotlight_depth_debug_mesh();
    void release_spotlight_depth_debug_mesh();
    void ensure_shadow_map_resources();
    void release_shadow_map_resources();

    /*
     * Rendering functions.
     */

    void render_shadow_map(
      const Scene& scene);
    void render_scene(
      const Scene& scene,
      const Camera& camera,
      const ViewportDisplaySettings& display_settings);

    void render_spotlight_depth_debug();

    void render_grid(
      const Camera& camera);

public:
    explicit Renderer(
      RenderDevice& device)
    : device{device}
    {
        create_grid_mesh();
        create_spotlight_depth_debug_mesh();
    }

    ~Renderer();

    [[nodiscard]]
    ShaderCache& get_shader_cache()
    {
        return shader_cache;
    }

    [[nodiscard]]
    float get_render_time() const noexcept
    {
        return render_time;
    }

    [[nodiscard]]
    const RendererStats& get_stats() const noexcept
    {
        return render_stats;
    }

    [[nodiscard]]
    const SortingBenchmarkResults& get_benchmark_results() const noexcept
    {
        return benchmark_results;
    }

    [[nodiscard]]
    bool is_benchmark_in_progress() const noexcept
    {
        return benchmark_state.active;
    }

    [[nodiscard]]
    std::size_t get_benchmark_current_iteration() const noexcept
    {
        return benchmark_state.current_iteration;
    }

    [[nodiscard]]
    std::size_t get_benchmark_target_iterations() const noexcept
    {
        return benchmark_state.target_iterations;
    }

    [[nodiscard]]
    bool is_benchmark_sorted_phase() const noexcept
    {
        return benchmark_state.sorted_phase;
    }

    [[nodiscard]]
    SortMode get_sort_mode() const noexcept
    {
        return sort_mode;
    }

    void set_sort_mode(SortMode mode) noexcept
    {
        sort_mode = mode;
    }

    [[nodiscard]]
    std::size_t get_depth_bin_count() const noexcept
    {
        return depth_bin_count;
    }

    void set_depth_bin_count(std::size_t n) noexcept
    {
        depth_bin_count = n > 0 ? n : 1;
    }

    [[nodiscard]]
    ShadowPcfMode get_shadow_pcf_mode() const noexcept
    {
        return shadow_pcf_mode;
    }

    void set_shadow_pcf_mode(ShadowPcfMode mode) noexcept
    {
        shadow_pcf_mode = mode;
    }

    void start_sorting_benchmark(
      Scene& scene,
      Viewport& viewport,
      std::size_t iterations = 100);

    void update_sorting_benchmark(
      Scene& scene,
      Viewport& viewport);

    /* Comparative benchmark API */
    void start_comparative_benchmark(
      Scene& scene,
      Viewport& viewport,
      std::size_t iterations = 100);

    [[nodiscard]]
    bool is_comparative_benchmark_in_progress() const noexcept
    {
        return comparative_state.active;
    }

    [[nodiscard]]
    const swr::vector<SortingBenchmarkResults>& get_comparative_results() const noexcept
    {
        return comparative_state.results;
    }

    void render(
      const Scene& scene,
      const Viewport& viewport);
};

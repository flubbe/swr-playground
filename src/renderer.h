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
#include <utility>

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

/*
 * Render queues.
 */

struct DrawSubmission
{
    float sort_depth{0.f};
    MeshHandle mesh_handle{};
    MaterialHandle material_handle{};
    ml::vec4 color{1.f, 1.f, 1.f, 1.f};
    ml::mat4x4 view_from_mesh;
    ShadowMapBinding shadow_map;
};

struct ShadowCasterSubmission
{
    MeshHandle mesh_handle{};
    ml::mat4x4 light_view_from_mesh;
};

struct ShadowCamera
{
    ml::mat4x4 proj{ml::mat4x4::identity()};
    ml::mat4x4 view{ml::mat4x4::identity()};
    const class SpotLight* light{nullptr};
};

/** Render queue sorting. */
struct RenderQueueSorter
{
    /** Virtual destructor. */
    virtual ~RenderQueueSorter() = default;

    /**
     * Sort the render queue.
     *
     * @param render_queue The render queue to sort.
     */
    virtual void sort(
      swr::vector<DrawSubmission>& render_queue) = 0;

    /** Get the sort mode. */
    [[nodiscard]]
    virtual SortMode get_sort_mode() const noexcept = 0;

    /** Get the name of the sorter. Forwards to the `RenderQueueSortFactory::get_name(...)`. */
    [[nodiscard]]
    virtual const char* get_name() const;
};

/** Standard dynamic `std::stable_sort` (O(N log N)). */
struct FullRenderQueueSorter final
: public RenderQueueSorter
{
    constexpr static SortMode mode = SortMode::FullSort;
    constexpr static const char* name = "Full Sort (O(N log N))";

    void sort(swr::vector<DrawSubmission>& render_queue) override
    {
        if(render_queue.empty())
        {
            return;
        }

        std::stable_sort(
          render_queue.begin(),
          render_queue.end(),
          [](const DrawSubmission& lhs, const DrawSubmission& rhs)
          {
              return lhs.sort_depth < rhs.sort_depth;
          });
    }

    [[nodiscard]]
    SortMode get_sort_mode() const noexcept override
    {
        return mode;
    }
};

/** Coarse Depth Binning Sorter (O(N)). Retains allocation buffers across frames. */
class BinRenderQueueSorter final
: public RenderQueueSorter
{
    std::size_t bin_count{8};

    // Pre-allocated reusable scratch buffers to eliminate per-frame dynamic allocations
    swr::vector<swr::vector<DrawSubmission>> scratch_bins;

public:
    constexpr static SortMode mode = SortMode::BinSort;
    constexpr static const char* name = "Bin Sort (O(N))";

    explicit BinRenderQueueSorter(
      std::size_t num_bins = 8)
    : bin_count{std::max(num_bins, 1uz)}
    {
        scratch_bins.resize(bin_count);
    }

    void set_bin_count(
      std::size_t count)
    {
        bin_count = std::max(count, 1uz);
        scratch_bins.resize(bin_count);
    }

    [[nodiscard]]
    std::size_t get_bin_count() const noexcept
    {
        return bin_count;
    }

    void sort(
      swr::vector<DrawSubmission>& render_queue) override
    {
        if(render_queue.empty())
        {
            return;
        }

        // Find depth bounds
        float min_depth = std::numeric_limits<float>::max();
        float max_depth = std::numeric_limits<float>::lowest();

        for(const auto& submission: render_queue)
        {
            min_depth = std::min(min_depth, submission.sort_depth);
            max_depth = std::max(max_depth, submission.sort_depth);
        }

        const float depth_range = max_depth - min_depth;
        const float bin_scale = (depth_range > 0.f)
                                  ? (static_cast<float>(bin_count) / depth_range)
                                  : 0.f;

        // Clear existing scratch bins without deallocating their memory capacity
        for(auto& bin: scratch_bins)
        {
            bin.clear();
        }

        // Distribute submissions into bins
        for(const auto& submission: render_queue)
        {
            int bin_index = static_cast<int>((submission.sort_depth - min_depth) * bin_scale);
            bin_index = std::clamp(bin_index, 0, static_cast<int>(bin_count) - 1);
            scratch_bins[bin_index].push_back(submission);
        }

        // Reconstruct submissions back into the main queue
        render_queue.clear();
        for(const auto& bin: scratch_bins)
        {
            for(const auto& submission: bin)
            {
                render_queue.push_back(submission);
            }
        }
    }

    [[nodiscard]]
    SortMode get_sort_mode() const noexcept override
    {
        return mode;
    }
};

/**
 * Factory for the render queue sorting algorithms.
 *
 * @note This only exists to provide a single source of truth associating
 *     algorithms, UI, enums and names.
 */
struct RenderQueueSortFactory
{
    /** Array of supported sort modes. */
    static constexpr std::array<SortMode, 2> supported_modes = {
      FullRenderQueueSorter::mode,
      BinRenderQueueSorter::mode};

    /**
     * Create a render queue sorter.
     *
     * @param mode The sorter to create.
     * @returns Returns the new sorter.
     */
    [[nodiscard]]
    static std::unique_ptr<RenderQueueSorter> create(
      SortMode mode)
    {
        switch(mode)
        {
        case FullRenderQueueSorter::mode:
            return std::make_unique<FullRenderQueueSorter>();

        case BinRenderQueueSorter::mode:
            return std::make_unique<BinRenderQueueSorter>();
        }

        std::unreachable();
    }

    /**
     * Get the name of a sort mode.
     *
     * @param mode The sort mode.
     * @returns Returns the name of a sort mode.
     */
    [[nodiscard]]
    static constexpr const char* get_name(
      SortMode mode) noexcept
    {
        switch(mode)
        {
        case FullRenderQueueSorter::mode:
            return FullRenderQueueSorter::name;
        case BinRenderQueueSorter::mode:
            return BinRenderQueueSorter::name;
        }

        std::unreachable();
    }
};

inline const char* RenderQueueSorter::get_name() const
{
    return RenderQueueSortFactory::get_name(
      get_sort_mode());
}

class Renderer final
{
    static constexpr int shadow_map_resolution = 1024;

    RenderDevice& device;
    ShaderCache shader_cache;

    float render_time{0.f};
    RendererStats render_stats;
    SortingBenchmarkResults benchmark_results{};
    SortingBenchmarkState benchmark_state{};
    ComparativeBenchmarkState comparative_state{};
    ShadowPcfMode shadow_pcf_mode{ShadowPcfMode::Off};

    MaterialHandle shadow_material{0};

    /*
     * Render queues.
     */

    swr::vector<DrawSubmission> render_queue;
    swr::vector<ShadowCasterSubmission> shadow_queue;

    std::unique_ptr<RenderQueueSorter> render_queue_sorter{
      std::make_unique<FullRenderQueueSorter>()};

    void begin_scene_pass(
      const Scene& scene,
      const Camera& camera,
      const ViewportDisplaySettings& display_settings);
    void end_scene_pass();

    void build_render_queue(
      const Scene& scene,
      const ViewportDisplaySettings& display_settings);
    void sort_render_queue(
      const ViewportDisplaySettings& display_settings);
    void execute_render_queue();

    bool begin_shadow_pass();
    void end_shadow_pass();

    void build_shadow_queue(
      const Scene& scene);
    void execute_shadow_queue();

    /*
     * Per-pass state/scratch.
     */

    ml::mat4x4 view;
    ml::mat4x4 projection;

    std::optional<ShadowCamera> shadow_camera;
    bool shadow_linear_filter;

    std::chrono::time_point<std::chrono::steady_clock> render_start_time{};

    void begin_render(
      const Scene& scene);
    void end_render();

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
        return render_queue_sorter->get_sort_mode();
    }

    void set_sort_mode(SortMode mode)
    {
        if(render_queue_sorter->get_sort_mode() == mode)
        {
            return;
        }

        render_queue_sorter = RenderQueueSortFactory::create(mode);
    }

    [[nodiscard]]
    std::size_t get_depth_bin_count() const noexcept
    {
        if(render_queue_sorter->get_sort_mode() == BinRenderQueueSorter::mode)
        {
            return static_cast<BinRenderQueueSorter*>(
                     render_queue_sorter.get())
              ->get_bin_count();
        }

        return 0;
    }

    void set_depth_bin_count(std::size_t n) noexcept
    {
        if(render_queue_sorter->get_sort_mode() == BinRenderQueueSorter::mode)
        {
            static_cast<BinRenderQueueSorter*>(
              render_queue_sorter.get())
              ->set_bin_count(n);
        }
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
      std::size_t iterations);

    void update_sorting_benchmark(
      Scene& scene,
      Viewport& viewport);

    /* Comparative benchmark API */
    void start_comparative_benchmark(
      Scene& scene,
      Viewport& viewport,
      std::size_t iterations);

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

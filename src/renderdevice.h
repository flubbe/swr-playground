/**
 * Software Rasterizer Playground.
 *
 * Render device.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <ml/all.h>
#include <swr/swr.h>
#include <swr/shaders.h>

#include "assets/texture.h"
#include "containers/unordered_map.h"
#include "containers/vector.h"
#include "meshes/mesh.h"
#include "render_types.h"
#include "shader_constants.h"

class Scene;
class Camera;

/** GPU-side mesh data. */
struct MeshGpuData
{
    /** Handle to the vertex buffer. */
    std::uint32_t vertices_handle{0};

    /** Handle to the normal buffer. */
    std::uint32_t normals_handle{0};

    /** Handle to the texture coordinate buffer. */
    std::optional<std::uint32_t> texcoords_handle;
};

/** A render material. */
struct Material
{
    /** Shader instance. */
    const swr::program_base* shader{nullptr};

    /** Shader handle. */
    std::uint32_t shader_handle{0};

    /** Bound 2D textures by texture unit index. */
    swr::vector<std::uint32_t> texture_handles{};
};

/** Camera-related shader uniforms. */
struct CameraUniforms
{
    /** Projection matrix. */
    ml::mat4x4 proj;

    /** View matrix. */
    ml::mat4x4 view;
};

/** Lighting-related shader uniforms. */
struct LightingUniforms
{
    /** Number of active directional lights. */
    int directional_light_count{0};

    /** Directional lights in camera/view space: xyz = direction, w = brightness. */
    std::array<ml::vec4, shader::max_lights> directional_light_dirs{};

    /** Number of active spot lights. */
    int spot_light_count{0};

    /** Spot light positions in camera/view space: xyz = position, w = range. */
    std::array<ml::vec4, shader::max_spot_lights> spot_light_positions{};

    /** Spot light directions in camera/view space: xyz = direction, w = brightness. */
    std::array<ml::vec4, shader::max_spot_lights> spot_light_directions{};

    /** Spot light parameters: x = cos(inner cone), y = cos(outer cone). */
    std::array<ml::vec4, shader::max_spot_lights> spot_light_params{};

    /** Spot light colors. */
    std::array<ml::vec4, shader::max_spot_lights> spot_light_colors{};
};

/** Material-related shader uniforms. */
struct MaterialUniforms
{
    /** Per-material base color. */
    ml::vec4 base_color{1.f, 1.f, 1.f, 1.f};
};

/** Shadow-related shader uniforms. */
struct ShadowUniforms
{
    bool enabled{false};
    ml::mat4x4 clip_from_mesh{ml::mat4x4::identity()};
    ml::vec4 params{0.f, 0.f, 0.f, 0.f};
};

/** Rasterizer state. */
struct RasterizerState
{
    /** Whether to show geometry as wireframes. */
    bool wireframe{false};

    /** Whether to enable face culling. */
    bool cull_face{true};

    bool operator==(const RasterizerState& other) const
    {
        return wireframe == other.wireframe
               && cull_face == other.cull_face;
    }
    bool operator!=(const RasterizerState& other) const
    {
        return !(*this == other);
    }
};

/** GPU-side shadow-map render target data. */
struct ShadowMapTargetGpuData
{
    std::uint32_t texture_handle{0};
    std::uint32_t framebuffer_handle{0};
    int width{0};
    int height{0};
};

/** Render device. */
class RenderDevice
{
    /** framebuffer width. */
    int width = 0;

    /** framebuffer height. */
    int height = 0;

    /** framebuffer data pointer. */
    std::uint32_t* data{nullptr};

    /** rasterizer context. */
    swr::context_handle context{nullptr};

    /** meshes. */
    swr::unordered_map<MeshHandle, MeshData> meshes;

    /** uploaded mesh data. */
    swr::unordered_map<MeshHandle, MeshGpuData> mesh_gpu_data;

    /** Mesh bounds. */
    swr::unordered_map<MeshHandle, MeshBounds> mesh_bounds;

    /** materials. */
    swr::unordered_map<MaterialHandle, Material> materials;

    /** shadow-map render targets. */
    swr::unordered_map<ShadowMapHandle, ShadowMapTargetGpuData> shadow_map_targets;

    /** state cache. */
    RasterizerState current_rasterizer_state;
    std::size_t current_bound_texture_count{0};
    std::optional<ShadowMapBinding> current_shadow_map_binding;
    ShadowMapHandle active_shadow_map_pass{};

    void apply_rasterizer_state(const RasterizerState& state);

    [[nodiscard]]
    const ShadowMapTargetGpuData* find_shadow_map_target(
      ShadowMapHandle handle) const;

protected:
    void initialize()
    {
        swr::SetClearColor(0, 0, 0, 1);
        swr::SetClearDepth(1.0f);
        swr::SetViewport(0, 0, width, height);
    }

    void release()
    {
        while(!meshes.empty())
        {
            delete_mesh(meshes.begin()->first);
        }
        while(!mesh_gpu_data.empty())
        {
            delete_mesh(mesh_gpu_data.begin()->first);
        }
        mesh_bounds.clear();

        while(!materials.empty())
        {
            delete_material(materials.begin()->first);
        }
        while(!shadow_map_targets.empty())
        {
            delete_shadow_map(shadow_map_targets.begin()->first);
        }

        if(context != nullptr)
        {
            swr::DestroyContext(context);
            context = nullptr;
        }
    }

public:
    RenderDevice(
      int width,
      int height)
    {
        resize(width, height);
    }

    ~RenderDevice()
    {
        release();
    }

    void resize(int width, int height);

    /*
     * getters.
     */

    [[nodiscard]]
    int get_width() const noexcept
    {
        return width;
    }

    [[nodiscard]]
    int get_height() const noexcept
    {
        return height;
    }

    [[nodiscard]]
    const std::uint32_t* get_data() const noexcept
    {
        return data;
    }

    [[nodiscard]]
    std::uint32_t* get_data() noexcept
    {
        return data;
    }

    // FIXME this accessor should not exist?
    const Material* get_material(MaterialHandle handle) const
    {
        auto it = materials.find(handle);
        if(it == materials.cend())
        {
            return nullptr;
        }
        return &it->second;
    }

    /*
     * resource management.
     */

    MeshHandle create_mesh(MeshData mesh);

    bool update_mesh(
      MeshHandle handle,
      MeshData mesh);

    [[nodiscard]]
    const MeshBounds* get_mesh_bounds(
      MeshHandle handle) const;

    // FIXME temporary?
    [[nodiscard]]
    std::size_t get_mesh_triangle_count(
      MeshHandle handle) const;

    void delete_mesh(MeshHandle handle);

    std::uint32_t create_texture(
      const assets::ImageRGBA8& image);

    void delete_texture(std::uint32_t handle);

    ShadowMapHandle create_shadow_map(
      int width,
      int height);

    void delete_shadow_map(ShadowMapHandle handle);

    MaterialHandle create_material(
      const swr::program_base& shader);

    MaterialHandle create_material(
      const swr::program_base& shader,
      std::span<const std::uint32_t> texture_handles);

    void delete_material(
      MaterialHandle handle,
      bool delete_textures = true);

    /*
     * begin/end frame.
     */

    void begin_frame()
    {
        swr::ClearColorBuffer();
        swr::ClearDepthBuffer();

        swr::SetState(swr::state::depth_test, true);
    }

    void end_frame()
    {
        swr::Present();
    }

    /*
     * bindings.
     */

    void bind_rasterizer_state(const RasterizerState& state);
    void bind_material(MaterialHandle handle);
    void bind_camera_uniforms(const CameraUniforms& uniforms);
    void bind_lighting_uniforms(const LightingUniforms& uniforms);
    void bind_material_uniforms(const MaterialUniforms& uniforms);
    void bind_shadow_map(const ShadowMapBinding& binding);
    void bind_shadow_uniforms(const ShadowUniforms& uniforms);

    void clear_shadow_map();

    void begin_shadow_map_pass(ShadowMapHandle handle);
    void end_shadow_map_pass();

    /*
     * drawing functions.
     */

    void draw_mesh(MeshHandle handle);
};

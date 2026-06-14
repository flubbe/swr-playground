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
#include <unordered_map>
#include <vector>

#include "ml/all.h"
#include "swr/swr.h"
#include "swr/shaders.h"

#include "assets/texture.h"
#include "mesh.h"
#include "shader.h"

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
    std::vector<std::uint32_t> texture_handles{};
};

/** Shader uniforms. */
struct Uniforms
{
    /** Projection matrix. */
    ml::mat4x4 proj;

    /** View matrix. */
    ml::mat4x4 view;

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
    std::unordered_map<std::uint32_t, MeshData> meshes;

    /** uploaded mesh data. */
    std::unordered_map<std::uint32_t, MeshGpuData> mesh_gpu_data;

    /** Mesh bounds. */
    std::unordered_map<std::uint32_t, MeshBounds> mesh_bounds;

    /** materials. */
    std::unordered_map<std::uint32_t, Material> materials;

    /** state cache. */
    RasterizerState current_rasterizer_state;
    std::size_t current_bound_texture_count{0};

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

    /*
     * resource management.
     */

    std::uint32_t create_mesh(MeshData mesh);

    bool update_mesh(
      std::uint32_t handle,
      MeshData mesh);

    [[nodiscard]]
    const MeshBounds* get_mesh_bounds(
      std::uint32_t handle) const;

    // FIXME temporary?
    [[nodiscard]]
    std::size_t get_mesh_triangle_count(
      std::uint32_t handle) const;

    void delete_mesh(std::uint32_t handle);

    std::uint32_t create_texture(
      const assets::ImageRgba8& image);

    void delete_texture(std::uint32_t handle);

    std::uint32_t create_material(
      const swr::program_base& shader);

    std::uint32_t create_material(
      const swr::program_base& shader,
      std::span<const std::uint32_t> texture_handles);

    void delete_material(std::uint32_t handle);

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
    void bind_material(std::uint32_t handle);
    void bind_uniforms(const Uniforms& uniforms);

    /*
     * drawing functions.
     */

    void draw_mesh(std::uint32_t handle);
};

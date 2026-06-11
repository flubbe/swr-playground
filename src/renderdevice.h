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
#include <unordered_map>
#include <vector>

#include "ml/all.h"
#include "swr/swr.h"
#include "swr/shaders.h"

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
};

/** A render material. */
struct Material
{
    /** Shader instance. */
    const swr::program_base* shader{nullptr};

    /** Shader handle. */
    std::uint32_t shader_handle{0};
};

/** Shader uniforms. */
struct Uniforms
{
    /** Projection matrix. */
    ml::mat4x4 proj;

    /** View matrix. */
    ml::mat4x4 view;

    /** Light position in camera/view space. */
    ml::vec4 light_pos;
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

    std::uint32_t create_material(
      const swr::program_base& shader);

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

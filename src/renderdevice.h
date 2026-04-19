/**
 * Software Rasterizer Playground.
 *
 * A render device.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "swr/swr.h"
#include "swr/shaders.h"

#include "shader.h"
#include "scene/gear.h"

class Scene;
class Camera;

struct MeshData
{
    std::vector<std::uint32_t> indices;

    std::vector<ml::vec4> vertices;
    std::uint32_t vertices_handle{0};

    std::vector<ml::vec4> normals;
    std::uint32_t normals_handle{0};
};

struct Material
{
    const swr::program_base* shader{nullptr};
    std::uint32_t shader_handle{0};
};

struct Uniforms
{
    ml::mat4x4 proj;
    ml::mat4x4 view;
    ml::vec4 light_dir;
};

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

    /** materials. */
    std::unordered_map<std::uint32_t, Material> materials;

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

    std::uint32_t create_mesh(
      std::vector<std::uint32_t> indices,
      std::vector<ml::vec4> vertices,
      std::vector<ml::vec4> normals);

    void delete_mesh(std::uint32_t handle);

    std::uint32_t create_material(
      const swr::program_base& shader);

    void delete_material(std::uint32_t handle);

    /*
     * begin/end frame.
     */

    void begin_frame(
      bool wireframe,
      bool cull_face)
    {
        swr::ClearColorBuffer();
        swr::ClearDepthBuffer();

        swr::SetState(swr::state::depth_test, true);

        if(wireframe)
        {
            swr::SetPolygonMode(swr::polygon_mode::line);
        }
        else
        {
            swr::SetPolygonMode(swr::polygon_mode::fill);
        }

        swr::SetState(swr::state::cull_face, cull_face);
    }

    void end_frame()
    {
        swr::Present();
    }

    /*
     * bindings.
     */

    void bind_material(std::uint32_t handle);
    void bind_uniforms(const Uniforms& uniforms);

    /*
     * drawing functions.
     */

    void draw_mesh(std::uint32_t handle);
};

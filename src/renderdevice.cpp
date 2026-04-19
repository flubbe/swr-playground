/**
 * Software Rasterizer Playground.
 *
 * Viewport Framebuffer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cmath>
#include <print>

#include "scene/camera.h"
#include "scene/scene.h"
#include "renderdevice.h"

void RenderDevice::resize(int width, int height)
{
    width = std::max(1, width);
    height = std::max(1, height);

    if(context == nullptr)
    {
        context = swr::CreateOffscreenContext(width, height);
        if(!swr::MakeContextCurrent(context))
        {
            throw std::runtime_error("MakeCurrentContext failed");
        }

        swr::GetContextInfo(
          context,
          reinterpret_cast<void**>(&data),
          &this->width,
          &this->height,
          nullptr);

        initialize();
    }
    else if(width != this->width
            || height != this->height)
    {
        swr::MakeContextCurrent(nullptr);
        swr::ResizeOffscreenContext(context, width, height);
        if(!swr::MakeContextCurrent(context))
        {
            throw std::runtime_error("MakeCurrentContext failed");
        }

        swr::GetContextInfo(
          context,
          reinterpret_cast<void**>(&data),
          &this->width,
          &this->height,
          nullptr);

        swr::SetViewport(0, 0, this->width, this->height);
    }
}

std::uint32_t RenderDevice::create_mesh(
  std::vector<std::uint32_t> indices,
  std::vector<ml::vec4> vertices,
  std::vector<ml::vec4> normals)
{
    std::uint32_t mesh_id = 0;
    while(meshes.find(mesh_id) != meshes.end())
    {
        ++mesh_id;
    }

    std::uint32_t vertices_handle = swr::CreateAttributeBuffer(vertices);
    std::uint32_t normals_handle = swr::CreateAttributeBuffer(normals);

    meshes.insert({mesh_id,
                   {.indices = std::move(indices),
                    .vertices = std::move(vertices),
                    .vertices_handle = vertices_handle,
                    .normals = std::move(normals),
                    .normals_handle = normals_handle}});

    return mesh_id;
}

void RenderDevice::delete_mesh(std::uint32_t handle)
{
    auto it = meshes.find(handle);
    if(it == meshes.end())
    {
        return;
    }

    swr::DeleteAttributeBuffer(it->second.normals_handle);
    swr::DeleteAttributeBuffer(it->second.vertices_handle);

    meshes.erase(it);
}

std::uint32_t RenderDevice::create_material(
  const swr::program_base& shader)
{
    std::uint32_t shader_handle = swr::RegisterShader(&shader);
    if(shader_handle == 0)
    {
        throw std::runtime_error("Unable to register shader");
    }

    std::uint32_t material_id = 0;
    while(materials.find(material_id) != materials.end())
    {
        ++material_id;
    }

    materials.insert({material_id, {.shader = &shader, .shader_handle = shader_handle}});
    return material_id;
}

void RenderDevice::delete_material(std::uint32_t handle)
{
    auto it = materials.find(handle);
    if(it == materials.end())
    {
        return;
    }

    swr::UnregisterShader(it->second.shader_handle);

    materials.erase(it);
}

void RenderDevice::bind_material(std::uint32_t handle)
{
    auto it = materials.find(handle);
    if(it == materials.end())
    {
        return;
    }

    swr::BindShader(it->second.shader_handle);
}

void RenderDevice::bind_uniforms(const Uniforms& uniforms)
{
    swr::BindUniform(0, uniforms.proj);
    swr::BindUniform(1, uniforms.view);
    swr::BindUniform(2, uniforms.light_dir);
}

void RenderDevice::draw_mesh(std::uint32_t handle)
{
    auto it = meshes.find(handle);
    if(it == meshes.end())
    {
        return;
    }

    swr::EnableAttributeBuffer(it->second.vertices_handle, 0);
    swr::EnableAttributeBuffer(it->second.normals_handle, 1);

    swr::DrawIndexedElements(
      swr::vertex_buffer_mode::triangles,
      it->second.indices.size(),
      it->second.indices);

    swr::DisableAttributeBuffer(it->second.vertices_handle);
    swr::DisableAttributeBuffer(it->second.normals_handle);
}
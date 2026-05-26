/**
 * Software Rasterizer Playground.
 *
 * Render device.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <cmath>
#include <print>
#include <utility>

#include "scene/camera.h"
#include "scene/scene.h"
#include "renderdevice.h"

void RenderDevice::resize(
  int width,
  int height)
{
    width = std::max(1, width);
    height = std::max(1, height);

    if(context == nullptr)
    {
        context = swr::CreateOffscreenContext(width, height);
        if(!swr::MakeContextCurrent(context))
        {
            throw std::runtime_error{"MakeCurrentContext failed"};
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
  MeshData mesh)
{
    const MeshBounds bounds = calculate_mesh_bounds(mesh);

    std::uint32_t mesh_id = 0;
    while(meshes.contains(mesh_id)
          || mesh_gpu_data.contains(mesh_id)
          || mesh_bounds.contains(mesh_id))
    {
        ++mesh_id;
    }

    MeshGpuData gpu_data{
      .vertices_handle = swr::CreateAttributeBuffer(mesh.vertices),
      .normals_handle = swr::CreateAttributeBuffer(mesh.normals),
    };

    meshes.emplace(mesh_id, std::move(mesh));
    mesh_gpu_data.emplace(mesh_id, gpu_data);
    mesh_bounds.emplace(mesh_id, bounds);

    return mesh_id;
}

bool RenderDevice::update_mesh(
  std::uint32_t handle,
  MeshData mesh)
{
    auto mesh_it = meshes.find(handle);
    auto gpu_it = mesh_gpu_data.find(handle);
    if(mesh_it == meshes.end()
       || gpu_it == mesh_gpu_data.end())
    {
        return false;
    }

    swr::DeleteAttributeBuffer(gpu_it->second.normals_handle);
    swr::DeleteAttributeBuffer(gpu_it->second.vertices_handle);

    gpu_it->second.vertices_handle = swr::CreateAttributeBuffer(mesh.vertices);
    gpu_it->second.normals_handle = swr::CreateAttributeBuffer(mesh.normals);
    mesh_it->second = std::move(mesh);
    mesh_bounds[handle] = calculate_mesh_bounds(mesh_it->second);

    return true;
}

const MeshBounds* RenderDevice::get_mesh_bounds(
  std::uint32_t handle) const
{
    auto it = mesh_bounds.find(handle);
    if(it == mesh_bounds.end())
    {
        return nullptr;
    }

    return &it->second;
}

std::size_t RenderDevice::get_mesh_triangle_count(
  std::uint32_t handle) const
{
    const auto mesh_it = meshes.find(handle);
    if(mesh_it == meshes.end()
       || mesh_it->second.primitive_type != PrimitiveType::Triangles)
    {
        return 0;
    }

    return mesh_it->second.indices.size() / 3;
}

void RenderDevice::delete_mesh(std::uint32_t handle)
{
    auto gpu_it = mesh_gpu_data.find(handle);
    if(gpu_it != mesh_gpu_data.end())
    {
        swr::DeleteAttributeBuffer(gpu_it->second.normals_handle);
        swr::DeleteAttributeBuffer(gpu_it->second.vertices_handle);

        mesh_gpu_data.erase(gpu_it);
    }

    meshes.erase(handle);
    mesh_bounds.erase(handle);
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
    while(materials.contains(material_id))
    {
        ++material_id;
    }

    materials.insert({material_id,
                      {.shader = &shader,
                       .shader_handle = shader_handle}});
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

void RenderDevice::bind_rasterizer_state(
  const RasterizerState& state)
{
    if(state == current_rasterizer_state)
    {
        return;
    }
    current_rasterizer_state = state;

    if(current_rasterizer_state.wireframe)
    {
        swr::SetPolygonMode(swr::polygon_mode::line);
    }
    else
    {
        swr::SetPolygonMode(swr::polygon_mode::fill);
    }

    swr::SetState(
      swr::state::cull_face,
      current_rasterizer_state.cull_face);
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
    auto gpu_it = mesh_gpu_data.find(handle);
    if(it == meshes.end() || gpu_it == mesh_gpu_data.end())
    {
        return;
    }

    const auto& mesh = it->second;
    const auto& gpu_data = gpu_it->second;

    swr::EnableAttributeBuffer(gpu_data.vertices_handle, 0);
    swr::EnableAttributeBuffer(gpu_data.normals_handle, 1);

    const auto mode =
      mesh.primitive_type == PrimitiveType::Lines
        ? swr::vertex_buffer_mode::lines
        : swr::vertex_buffer_mode::triangles;

    swr::DrawIndexedElements(
      mode,
      mesh.indices.size(),
      mesh.indices);

    swr::DisableAttributeBuffer(gpu_data.vertices_handle);
    swr::DisableAttributeBuffer(gpu_data.normals_handle);
}

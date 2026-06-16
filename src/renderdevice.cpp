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
#include <span>
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
            throw std::runtime_error{"MakeCurrentContext failed"};
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
      .texcoords_handle =
        mesh.texcoords.empty()
          ? std::nullopt
          : std::make_optional(
              swr::CreateAttributeBuffer(mesh.texcoords)),
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

    if(gpu_it->second.texcoords_handle.has_value())
    {
        swr::DeleteAttributeBuffer(*gpu_it->second.texcoords_handle);
    }
    swr::DeleteAttributeBuffer(gpu_it->second.normals_handle);
    swr::DeleteAttributeBuffer(gpu_it->second.vertices_handle);

    gpu_it->second.vertices_handle = swr::CreateAttributeBuffer(mesh.vertices);
    gpu_it->second.normals_handle = swr::CreateAttributeBuffer(mesh.normals);
    gpu_it->second.texcoords_handle =
      mesh.texcoords.empty()
        ? std::nullopt
        : std::make_optional(
            swr::CreateAttributeBuffer(mesh.texcoords));
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
        if(gpu_it->second.texcoords_handle.has_value())
        {
            swr::DeleteAttributeBuffer(*gpu_it->second.texcoords_handle);
        }
        swr::DeleteAttributeBuffer(gpu_it->second.normals_handle);
        swr::DeleteAttributeBuffer(gpu_it->second.vertices_handle);

        mesh_gpu_data.erase(gpu_it);
    }

    meshes.erase(handle);
    mesh_bounds.erase(handle);
}

std::uint32_t RenderDevice::create_texture(
  const assets::ImageRgba8& image)
{
    if(image.width <= 0
       || image.height <= 0
       || image.pixels.size()
            != static_cast<std::size_t>(image.width * image.height * 4))
    {
        throw std::runtime_error{"Unable to create texture from invalid RGBA8 image data"};
    }

    const std::uint32_t texture_id = swr::CreateTexture();
    if(texture_id == 0)
    {
        throw std::runtime_error{"Unable to create texture handle"};
    }

    swr::ActiveTexture(swr::texture_0);
    swr::BindTexture(swr::texture_target::texture_2d, texture_id);
    swr::SetImage(
      texture_id,
      0,
      static_cast<std::size_t>(image.width),
      static_cast<std::size_t>(image.height),
      swr::pixel_format::rgba8888,
      image.pixels);
    if(swr::GetLastError() != swr::error::none)
    {
        swr::ReleaseTexture(texture_id);
        throw std::runtime_error{"Unable to upload texture image"};
    }

    swr::SetTextureWrapMode(
      texture_id,
      swr::wrap_mode::repeat,
      swr::wrap_mode::repeat);
    swr::SetTextureMinificationFilter(swr::texture_filter::linear);
    swr::SetTextureMagnificationFilter(swr::texture_filter::linear);
    swr::BindTexture(swr::texture_target::texture_2d, 0);

    return texture_id;
}

void RenderDevice::delete_texture(std::uint32_t handle)
{
    if(handle != 0)
    {
        swr::ReleaseTexture(handle);
    }
}

std::uint32_t RenderDevice::create_material(
  const swr::program_base& shader)
{
    return create_material(
      shader,
      std::span<const std::uint32_t>{});
}

std::uint32_t RenderDevice::create_material(
  const swr::program_base& shader,
  std::span<const std::uint32_t> texture_handles)
{
    std::uint32_t shader_handle = swr::RegisterShader(&shader);
    if(shader_handle == 0)
    {
        throw std::runtime_error{"Unable to register shader"};
    }

    std::uint32_t material_id = 0;
    while(materials.contains(material_id))
    {
        ++material_id;
    }

    materials.insert({material_id,
                      {.shader = &shader,
                       .shader_handle = shader_handle,
                       .texture_handles = std::vector<std::uint32_t>(
                         texture_handles.begin(),
                         texture_handles.end())}});
    return material_id;
}

void RenderDevice::delete_material(
  std::uint32_t handle,
  bool delete_textures)
{
    auto it = materials.find(handle);
    if(it == materials.end())
    {
        return;
    }

    swr::UnregisterShader(it->second.shader_handle);
    if(delete_textures)
    {
        for(const std::uint32_t texture_handle: it->second.texture_handles)
        {
            if(texture_handle != 0)
            {
                delete_texture(texture_handle);
            }
        }
    }

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

    const std::size_t texture_count = it->second.texture_handles.size();
    swr::SetState(
      swr::state::texture,
      texture_count > 0);

    for(std::size_t unit = 0; unit < texture_count; ++unit)
    {
        swr::ActiveTexture(static_cast<std::uint32_t>(unit));
        swr::BindTexture(
          swr::texture_target::texture_2d,
          it->second.texture_handles[unit]);
    }
    for(std::size_t unit = texture_count; unit < current_bound_texture_count; ++unit)
    {
        swr::ActiveTexture(static_cast<std::uint32_t>(unit));
        swr::BindTexture(
          swr::texture_target::texture_2d,
          0);
    }
    current_bound_texture_count = texture_count;
}

void RenderDevice::bind_camera_uniforms(const CameraUniforms& uniforms)
{
    swr::BindUniform(
      static_cast<std::uint32_t>(shader::camera_projection_uniform_index),
      uniforms.proj);
    swr::BindUniform(
      static_cast<std::uint32_t>(shader::camera_view_uniform_index),
      uniforms.view);
}

void RenderDevice::bind_lighting_uniforms(const LightingUniforms& uniforms)
{
    swr::BindUniform(
      static_cast<std::uint32_t>(shader::directional_light_count_uniform_index),
      uniforms.directional_light_count);
    for(std::size_t light_index = 0;
        light_index < uniforms.directional_light_dirs.size();
        ++light_index)
    {
        swr::BindUniform(
          static_cast<std::uint32_t>(
            shader::directional_light_uniform_index(light_index)),
          uniforms.directional_light_dirs[light_index]);
    }
    swr::BindUniform(
      static_cast<std::uint32_t>(shader::spot_light_count_uniform_index),
      uniforms.spot_light_count);
    for(std::size_t light_index = 0;
        light_index < uniforms.spot_light_positions.size();
        ++light_index)
    {
        swr::BindUniform(
          static_cast<std::uint32_t>(shader::spot_light_uniform_index(
            light_index,
            shader::spot_light_position_uniform_offset)),
          uniforms.spot_light_positions[light_index]);
        swr::BindUniform(
          static_cast<std::uint32_t>(shader::spot_light_uniform_index(
            light_index,
            shader::spot_light_direction_uniform_offset)),
          uniforms.spot_light_directions[light_index]);
        swr::BindUniform(
          static_cast<std::uint32_t>(shader::spot_light_uniform_index(
            light_index,
            shader::spot_light_params_uniform_offset)),
          uniforms.spot_light_params[light_index]);
        swr::BindUniform(
          static_cast<std::uint32_t>(shader::spot_light_uniform_index(
            light_index,
            shader::spot_light_color_uniform_offset)),
          uniforms.spot_light_colors[light_index]);
    }
}

void RenderDevice::bind_material_uniforms(const MaterialUniforms& uniforms)
{
    swr::BindUniform(
      static_cast<std::uint32_t>(shader::material_color_uniform_index),
      uniforms.base_color);
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
    if(gpu_data.texcoords_handle.has_value())
    {
        swr::EnableAttributeBuffer(*gpu_data.texcoords_handle, 2);
    }

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
    if(gpu_data.texcoords_handle.has_value())
    {
        swr::DisableAttributeBuffer(*gpu_data.texcoords_handle);
    }
}

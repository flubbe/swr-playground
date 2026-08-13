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
#include <bit>
#include <cmath>
#include <span>
#include <utility>
#include <type_traits>

#include <gsl/gsl>

#include "scene/camera.h"
#include "scene/scene.h"
#include "renderdevice.h"

void RenderDevice::apply_rasterizer_state(const RasterizerState& state)
{
    if(state.wireframe)
    {
        swr::SetPolygonMode(swr::polygon_mode::line);
    }
    else
    {
        swr::SetPolygonMode(swr::polygon_mode::fill);
    }

    swr::SetState(
      swr::state::cull_face,
      state.cull_face);
}

const ShadowMapTargetGpuData* RenderDevice::find_shadow_map_target(
  ShadowMapHandle handle) const
{
    const auto it = shadow_map_targets.find(handle);
    if(it == shadow_map_targets.end())
    {
        return nullptr;
    }

    return &it->second;
}

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

MeshHandle RenderDevice::create_mesh(
  const MeshData& mesh)
{
    MeshHandle mesh_id{1};
    while(meshes.contains(mesh_id))
    {
        ++mesh_id;
    }

    MeshGpuData gpu_data{
      .primitive_type = mesh.primitive_type,
      .indices = mesh.indices,
      .vertices_handle = {swr::CreateAttributeBuffer(mesh.vertices)},
      .normals_handle = {swr::CreateAttributeBuffer(mesh.normals)},
      .texcoords_handle =
        mesh.texcoords.empty()
          ? std::nullopt
          : std::make_optional(
              TexCoordBufferHandle{
                swr::CreateAttributeBuffer(mesh.texcoords)}),
    };

    meshes.emplace(mesh_id, gpu_data);
    return mesh_id;
}

bool RenderDevice::update_mesh(
  MeshHandle handle,
  const MeshData& mesh)
{
    auto mesh_it = meshes.find(handle);
    if(mesh_it == meshes.end())
    {
        return false;
    }

    if(mesh_it->second.texcoords_handle.has_value())
    {
        swr::DeleteAttributeBuffer(mesh_it->second.texcoords_handle.value().value);
    }
    swr::DeleteAttributeBuffer(mesh_it->second.normals_handle.value);
    swr::DeleteAttributeBuffer(mesh_it->second.vertices_handle.value);

    mesh_it->second.primitive_type = mesh.primitive_type;
    mesh_it->second.indices = mesh.indices;
    mesh_it->second.vertices_handle = {swr::CreateAttributeBuffer(mesh.vertices)};
    mesh_it->second.normals_handle = {swr::CreateAttributeBuffer(mesh.normals)};
    mesh_it->second.texcoords_handle =
      mesh.texcoords.empty()
        ? std::nullopt
        : std::make_optional(
            TexCoordBufferHandle{
              swr::CreateAttributeBuffer(mesh.texcoords)});

    return true;
}

void RenderDevice::delete_mesh(MeshHandle handle)
{
    auto mesh_it = meshes.find(handle);
    if(mesh_it != meshes.end())
    {
        if(mesh_it->second.texcoords_handle.has_value())
        {
            swr::DeleteAttributeBuffer(mesh_it->second.texcoords_handle.value().value);
        }
        swr::DeleteAttributeBuffer(mesh_it->second.normals_handle.value);
        swr::DeleteAttributeBuffer(mesh_it->second.vertices_handle.value);

        meshes.erase(mesh_it);
    }

    meshes.erase(handle);
}

ShaderHandle RenderDevice::create_shader(
  const swr::program_base& shader)
{
    auto shader_id = swr::RegisterShader(&shader);
    if(!shader_id)
    {
        throw std::runtime_error{
          "Shader registration failed."};
    }

    bool success = false;
    auto rollback = gsl::finally(
      [&]()
      {
          if(!success)
          {
              swr::UnregisterShader(shader_id);
          }
      });

    auto shader_handle = ShaderHandle{shader_id};
    auto [it, inserted] = shaders.insert({shader_handle, &shader});
    if(!inserted)
    {
        throw std::runtime_error{
          "Could not insert shader."};
    }
    success = true;

    return shader_handle;
}

void RenderDevice::delete_shader(
  ShaderHandle handle)
{
    auto it = shaders.find(handle);
    if(it != shaders.end())
    {
        swr::UnregisterShader(handle.value);
        shaders.erase(it);
    }
}

TextureHandle RenderDevice::create_texture(
  const assets::ImageRGBA8& image)
{
    if(image.width <= 0
       || image.height <= 0
       || image.pixels.size()
            != static_cast<std::size_t>(image.width * image.height * 4))
    {
        throw std::runtime_error{
          "Unable to create texture from invalid RGBA8 image data"};
    }

    // Validate image size.
    // FIXME Should this be done in the graphics API?
    if(!std::has_single_bit(static_cast<unsigned int>(image.width))
       || !std::has_single_bit(static_cast<unsigned int>(image.height)))
    {
        throw std::runtime_error{
          std::format(
            "Image dimensions ({}, {}) are not powers of two.",
            image.width,
            image.height)};
    }

    const std::uint32_t texture_id = swr::CreateTexture();
    if(texture_id == 0)
    {
        throw std::runtime_error{"Unable to create texture handle"};
    }

    bool success = false;
    auto rollback = gsl::finally(
      [&]()
      {
          if(!success)
          {
              swr::ReleaseTexture(texture_id);
          }
      });

    swr::ActiveTexture(swr::texture_0);
    swr::BindTexture(swr::texture_target::texture_2d, texture_id);
    swr::SetImage(
      texture_id,
      0,
      static_cast<std::size_t>(image.width),
      static_cast<std::size_t>(image.height),
      swr::pixel_format::rgba8888,
      {image.pixels.begin(), image.pixels.end()});    // FIXME Copies. SetImage should take a span.
    if(swr::GetLastError() != swr::error::none)
    {
        throw std::runtime_error{"Unable to upload texture image"};
    }

    swr::SetTextureWrapMode(
      texture_id,
      swr::wrap_mode::repeat,
      swr::wrap_mode::repeat);
    swr::SetTextureMinificationFilter(swr::texture_filter::linear);
    swr::SetTextureMagnificationFilter(swr::texture_filter::linear);
    swr::BindTexture(swr::texture_target::texture_2d, 0);

    success = true;
    return {texture_id};
}

void RenderDevice::delete_texture(TextureHandle handle)
{
    if(handle != 0)
    {
        swr::ReleaseTexture(handle.value);
    }
}

ShadowMapHandle RenderDevice::create_shadow_map(
  int width,
  int height)
{
    ShadowMapTargetGpuData gpu_data{};
    gpu_data.width = std::max(1, width);
    gpu_data.height = std::max(1, height);
    gpu_data.texture_handle = {swr::CreateTexture()};
    if(gpu_data.texture_handle == 0)
    {
        throw std::runtime_error{"Unable to create shadow-map texture handle"};
    }

    bool success = false;
    auto rollback_texture = gsl::finally(
      [&]()
      {
          if(!success)
          {
              swr::ReleaseTexture(gpu_data.texture_handle.value);
          }
      });

    swr::ActiveTexture(shader::shadow_map_sampler_unit);
    swr::BindTexture(
      swr::texture_target::texture_2d,
      gpu_data.texture_handle.value);
    swr::SetImage(
      gpu_data.texture_handle.value,
      0,
      static_cast<std::size_t>(gpu_data.width),
      static_cast<std::size_t>(gpu_data.height),
      swr::pixel_format::depth32f,
      {});
    swr::SetTextureWrapMode(
      gpu_data.texture_handle.value,
      swr::wrap_mode::clamp_to_edge,
      swr::wrap_mode::clamp_to_edge);
    swr::SetTextureMinificationFilter(swr::texture_filter::nearest);
    swr::SetTextureMagnificationFilter(swr::texture_filter::nearest);
    swr::BindTexture(swr::texture_target::texture_2d, 0);
    swr::SetTextureCompareMode(
      gpu_data.texture_handle.value,
      swr::texture_compare_mode::ref_to_texture);
    swr::SetTextureCompareFunc(
      gpu_data.texture_handle.value,
      swr::comparison_func::less_equal);
    if(swr::GetLastError() != swr::error::none)
    {
        throw std::runtime_error{"Unable to allocate shadow-map texture"};
    }

    gpu_data.framebuffer_handle = {swr::CreateFramebufferObject()};
    if(gpu_data.framebuffer_handle == 0)
    {
        throw std::runtime_error{"Unable to create shadow-map framebuffer"};
    }

    auto rollback_framebuffer = gsl::finally(
      [&]()
      {
          if(!success)
          {
              swr::ReleaseFramebufferObject(gpu_data.framebuffer_handle.value);
          }
      });

    swr::FramebufferTexture(
      gpu_data.framebuffer_handle.value,
      swr::framebuffer_attachment::depth_attachment,
      gpu_data.texture_handle.value,
      0);
    if(swr::GetLastError() != swr::error::none)
    {
        throw std::runtime_error{"Unable to attach shadow-map depth texture"};
    }

    ShadowMapHandle handle{1};
    while(shadow_map_targets.contains(handle))
    {
        ++handle.value;
    }

    shadow_map_targets.emplace(handle, gpu_data);

    success = true;
    return handle;
}

void RenderDevice::delete_shadow_map(ShadowMapHandle handle)
{
    const auto it = shadow_map_targets.find(handle);
    if(it == shadow_map_targets.end())
    {
        return;
    }

    if(active_shadow_map_pass == handle)
    {
        end_shadow_map_pass();
    }

    if(current_shadow_map_binding.has_value()
       && current_shadow_map_binding->handle == handle)
    {
        current_shadow_map_binding.reset();
    }

    if(it->second.framebuffer_handle != 0)
    {
        swr::ReleaseFramebufferObject(it->second.framebuffer_handle.value);
    }
    if(it->second.texture_handle != 0)
    {
        delete_texture(it->second.texture_handle);
    }

    shadow_map_targets.erase(it);
}

MaterialHandle RenderDevice::create_material(
  const Material& material)
{
    MaterialHandle material_id{1};
    while(materials.contains(material_id))
    {
        ++material_id.value;
    }

    materials.insert({material_id, material});
    return material_id;
}

void RenderDevice::delete_material(
  MaterialHandle handle)
{
    auto it = materials.find(handle);
    if(it == materials.end())
    {
        return;
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
    apply_rasterizer_state(current_rasterizer_state);
}

void RenderDevice::bind_material(MaterialHandle handle)
{
    auto it = materials.find(handle);
    if(it == materials.end())
    {
        return;
    }

    swr::BindShader(it->second.shader_handle.value);

    const auto material_texture_count =
      (it->second.base_color_handle ? 1 : 0)
      + (it->second.normal_map_handle ? 1 : 0);
    const bool has_material_texture = material_texture_count > 0;

    const ShadowMapTargetGpuData* shadow_target =
      current_shadow_map_binding.has_value()
        ? find_shadow_map_target(current_shadow_map_binding->handle)
        : nullptr;
    const bool has_shadow_texture =
      current_shadow_map_binding.has_value()
      && current_shadow_map_binding->enabled
      && shadow_target != nullptr
      && shadow_target->texture_handle != 0;
    swr::SetState(
      swr::state::texture,
      has_material_texture || has_shadow_texture);

    swr::ActiveTexture(shader::base_color_sampler_unit);
    swr::BindTexture(
      swr::texture_target::texture_2d,
      it->second.base_color_handle.value);    // might be zero for inactive textures

    swr::ActiveTexture(shader::normal_map_sampler_unit);
    swr::BindTexture(
      swr::texture_target::texture_2d,
      it->second.normal_map_handle.value);    // might be zero for inactive textures

    if(has_shadow_texture)
    {
        swr::ActiveTexture(shader::shadow_map_sampler_unit);
        swr::BindTexture(
          swr::texture_target::texture_2d,
          shadow_target->texture_handle.value);
        const swr::texture_filter filter =
          current_shadow_map_binding->linear_filter
            ? swr::texture_filter::linear
            : swr::texture_filter::nearest;
        swr::SetTextureMinificationFilter(filter);
        swr::SetTextureMagnificationFilter(filter);
    }
    current_bound_texture_count = material_texture_count
                                  + (has_shadow_texture ? 1 : 0);
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

void RenderDevice::bind_shadow_map(const ShadowMapBinding& binding)
{
    if(binding.enabled
       && binding.handle
       && find_shadow_map_target(binding.handle) != nullptr)
    {
        current_shadow_map_binding = binding;
        return;
    }

    current_shadow_map_binding.reset();
}

void RenderDevice::clear_shadow_map()
{
    current_shadow_map_binding.reset();
}

void RenderDevice::bind_shadow_uniforms(const ShadowUniforms& uniforms)
{
    swr::BindUniform(
      static_cast<std::uint32_t>(shader::shadow_map_enabled_uniform_index),
      uniforms.enabled ? 1 : 0);
    swr::BindUniform(
      static_cast<std::uint32_t>(shader::shadow_map_matrix_uniform_index),
      uniforms.clip_from_mesh);
    swr::BindUniform(
      static_cast<std::uint32_t>(shader::shadow_map_params_uniform_index),
      uniforms.params);
}

void RenderDevice::begin_shadow_map_pass(ShadowMapHandle handle)
{
    const ShadowMapTargetGpuData* target = find_shadow_map_target(handle);
    if(target == nullptr)
    {
        return;
    }

    active_shadow_map_pass = handle;
    swr::BindFramebufferObject(
      swr::framebuffer_target::draw,
      target->framebuffer_handle.value);
    swr::SetViewport(0, 0, target->width, target->height);
    swr::SetClearDepth(1.f);
    swr::ClearDepthBuffer();
    swr::SetState(swr::state::depth_test, true);
    swr::SetState(swr::state::depth_write, true);
    swr::SetState(swr::state::texture, false);
    swr::SetState(swr::state::cull_face, true);
    swr::SetState(swr::state::polygon_offset_fill, true);
    swr::PolygonOffset(1.5f, 2.f);
    swr::SetPolygonMode(swr::polygon_mode::fill);
}

void RenderDevice::end_shadow_map_pass()
{
    if(!active_shadow_map_pass)
    {
        return;
    }

    swr::SetState(swr::state::polygon_offset_fill, false);
    swr::BindFramebufferObject(swr::framebuffer_target::draw, 0);
    swr::SetViewport(0, 0, width, height);
    swr::SetClearColor(0.f, 0.f, 0.f, 1.f);
    apply_rasterizer_state(current_rasterizer_state);
    active_shadow_map_pass = {};
}

void RenderDevice::draw_mesh(MeshHandle handle)
{
    auto it = meshes.find(handle);
    if(it == meshes.end())
    {
        return;
    }

    const auto& mesh = it->second;

    swr::EnableAttributeBuffer(mesh.vertices_handle.value, 0);
    swr::EnableAttributeBuffer(mesh.normals_handle.value, 1);
    if(mesh.texcoords_handle.has_value())
    {
        swr::EnableAttributeBuffer(mesh.texcoords_handle.value().value, 2);
    }

    const auto mode =
      mesh.primitive_type == PrimitiveType::Lines
        ? swr::vertex_buffer_mode::lines
        : swr::vertex_buffer_mode::triangles;

    swr::DrawIndexedElements(
      mode,
      mesh.indices.size(),
      mesh.indices);

    swr::DisableAttributeBuffer(mesh.vertices_handle.value);
    swr::DisableAttributeBuffer(mesh.normals_handle.value);
    if(mesh.texcoords_handle.has_value())
    {
        swr::DisableAttributeBuffer(mesh.texcoords_handle.value().value);
    }
}

/**
 * Software Rasterizer Playground.
 *
 * Material management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <future>
#include <optional>
#include <stdexcept>

#include <gsl/gsl>

#include "materialmanager.h"
#include "renderdevice.h"
#include "shader_cache.h"
#include "shader_factory.h"
#include "texture_cache.h"

MaterialHandle ResolvableMaterial::resolve()
{
    if(state->resolved_handle.has_value())
    {
        return *state->resolved_handle;
    }

    MaterialResources loaded = state->resources.future.get();

    Material material;

    bool success = false;
    auto rollback = gsl::finally(
      [&]()
      {
          if(!success)
          {
              if(state->resolved_handle.has_value())
              {
                  state->render_device.delete_material(
                    state->resolved_handle.value());
              }

              // TODO Shader release is handled by the cache.

              // TODO These should probably be released through
              //      the texture cache once textures are cached.
              for(const auto& handle: material.texture_handles)
              {
                  state->render_device.delete_texture(handle);
              }
          }
      });

    material.shader_handle = state->shader_cache.load(
      loaded.description.shader,
      loaded.shader);

    for(const auto& texture: loaded.textures)
    {
        material.texture_handles.push_back(
          state->render_device.create_texture(texture));
    }

    state->resolved_handle = state->render_device.create_material(material);
    success = true;

    return *state->resolved_handle;
}

ResolvableMaterial MaterialManager::load(
  std::string_view key,
  std::string_view json)
{
    if(auto it = material_map.find(key);
       it != material_map.end())
    {
        return it->second;
    }

    // The material needs to be loaded. We delegate everything
    // to a task.

    auto submission = task_system.submit(
      [this, json = swr::string{json}](
        task_system::TaskExecutionContext& context) mutable -> MaterialResources
      {
          if(context.is_cancel_requested())
          {
              throw task_system::TaskCancelledError{};
          }

          MaterialResources resources;

          // Load shader. The factory call corresponds to loading the shader data.
          // TODO When allowing shader registrations during runtime, the factory call
          //      has to be made thread safe.
          resources.description = assets::load_material(json);
          resources.shader = shader_factory.get(resources.description.shader);
          if(resources.shader == nullptr)
          {
              // TODO Handle failure downstream
              throw task_system::TaskCancelledError{};
          }

          // Load textures.
          resources.textures.clear();
          resources.textures.reserve(resources.description.textures.size());
          for(const auto& texture: resources.description.textures)
          {
              if(context.is_cancel_requested())
              {
                  throw task_system::TaskCancelledError{};
              }

              // TODO Async texture asset cache to deduplicate concurrent loads of the same texture.
              resources.textures.emplace_back(
                assets::load_texture_rgba8(texture));
          }

          return resources;
      });

    ResolvableMaterial material{
      device,
      shader_cache,
      std::move(submission)};

    material_map.emplace(
      swr::string{key},
      material);

    return material;
}

// FIXME This invalidates the material, but already-loaded materials
//       can still exist. These become invalid.
bool MaterialManager::delete_material(
  std::string_view key)
{
    if(auto it = material_map.find(key);
       it != material_map.end())
    {
        if(it->second.is_resolved())
        {
            device.delete_material(it->second.resolve());
        }

        material_map.erase(it);
        return true;
    }

    return false;
}

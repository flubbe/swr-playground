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
    if(!state)
    {
        throw std::logic_error{
          "Cannot resolve an invalid material."};
    }

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

              // TODO shader release is handled by the cache?

              // TODO These should probably be released through
              //      the texture cache once textures are cached.
              for(const auto& handle: material.texture_handles)
              {
                  state->render_device.delete_texture(handle);
              }
          }
      });

    material.shader_handle = state->shader_cache.load(
      loaded.shader_key,
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

    // the material needs to be loaded. we delegate everything
    // to a task.

    auto submission = task_system.submit(
      // FIXME copy JSON?
      [this, json = swr::string{json}](task_system::TaskExecutionContext& context) mutable -> MaterialResources
      {
          if(context.is_cancel_requested())
          {
              throw task_system::TaskCancelledError{};
          }

          MaterialResources resources;

          auto desc = assets::load_material(json);

          // Load shader. The factory call corresponds to loading the shader data.
          resources.shader_key = desc.shader;
          resources.shader = shader_factory.get(desc.shader);

          // Load textures.
          resources.textures.clear();
          resources.textures.reserve(desc.textures.size());
          for(const auto& texture: desc.textures)
          {
              // TODO should there be a cache here?
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

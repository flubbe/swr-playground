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

MaterialEntry::~MaterialEntry()
{
    if(resolved_handle.has_value())
    {
        device.delete_material(
          resolved_handle.value());
    }
}

MaterialHandle MaterialEntry::resolve()
{
    if(resolved_handle.has_value())
    {
        return *resolved_handle;
    }

    MaterialResources loaded = resources.future.get();

    Material material;

    bool success = false;
    auto rollback = gsl::finally(
      [&]()
      {
          if(!success)
          {
              if(resolved_handle.has_value())
              {
                  device.delete_material(
                    resolved_handle.value());
              }

              // TODO Shader release is handled by the cache.

              textures.clear();
          }
      });

    material.shader_handle = shader_cache.load(
      loaded.description.shader,
      loaded.shader);

    for(const auto& texture: loaded.textures)
    {
        const std::uint64_t hash = TextureCache::compute_hash(texture);
        const swr::string generated_key =
          swr::format("hash://{:016x}", hash);

        auto texture_ref = texture_cache.load(
          generated_key,
          texture);

        textures.push_back(texture_ref);
        material.texture_handles.push_back(
          texture_ref.get());
    }

    resolved_handle = device.create_material(material);
    success = true;

    return *resolved_handle;
}

ResolvableMaterial MaterialManager::load(
  std::string_view key,
  std::string_view json)
{
    if(auto it = material_cache.find(key);
       it != material_cache.end())
    {
        return ResolvableMaterial{it->second};
    }

    // The material needs to be loaded. We delegate everything
    // to a task.

    auto submission = task_system.submit(
      [shader_factory = &shader_factory,
       json = swr::string{json}](
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
          resources.shader = shader_factory->get(resources.description.shader);
          if(resources.shader == nullptr)
          {
              // TODO Handle failure downstream
              throw task_system::TaskCancelledError{};
          }

          // Load textures.
          resources.textures.reserve(resources.description.textures.size());
          for(const auto& texture: resources.description.textures)
          {
              if(context.is_cancel_requested())
              {
                  throw task_system::TaskCancelledError{};
              }

              resources.textures.emplace_back(
                assets::load_texture_rgba8(texture));
          }

          return resources;
      });

    auto material = std::make_shared<MaterialEntry>(
      device,
      shader_cache,
      texture_cache,
      std::move(submission));

    material_cache.emplace(
      swr::string{key},
      material);

    return ResolvableMaterial{material};
}

bool MaterialManager::delete_material(
  std::string_view key)
{
    return material_cache.erase(swr::string{key}) != 0;
}

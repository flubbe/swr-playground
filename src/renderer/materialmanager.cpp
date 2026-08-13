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
#include "logging.h"

namespace
{

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"Materials"};
    return logger;
}

}    // namespace

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

              base_color.reset();
              normal_map.reset();
          }
      });

    material.shader_handle = shader_cache.load(
      loaded.description.shader,
      loaded.shader);

    if(loaded.base_color.has_value())
    {
        const std::uint64_t hash = TextureCache::compute_hash(*loaded.base_color);
        const swr::string generated_key =
          swr::format("hash://{:016x}", hash);

        base_color = texture_cache.load(
          generated_key,
          *loaded.base_color);
        material.base_color_handle = base_color->get();
    }

    if(loaded.normal_map.has_value())
    {
        const std::uint64_t hash = TextureCache::compute_hash(*loaded.normal_map);
        const swr::string generated_key =
          swr::format("hash://{:016x}", hash);

        normal_map = texture_cache.load(
          generated_key,
          *loaded.normal_map);
        material.normal_map_handle = normal_map->get();
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
       json = swr::string{json},
       key = swr::string{key}](
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

          /*
           * Load textures.
           */

          if(resources.description.base_color.has_value())
          {
              if(context.is_cancel_requested())
              {
                  throw task_system::TaskCancelledError{};
              }

              resources.base_color = assets::load_texture_rgba8(
                *resources.description.base_color);
          }

          if(resources.description.normal_map.has_value())
          {
              if(context.is_cancel_requested())
              {
                  throw task_system::TaskCancelledError{};
              }

              const assets::NormalMapDesc& normal_map =
                *resources.description.normal_map;
              resources.normal_map = assets::load_normal_map_rgba8(
                normal_map.path,
                normal_map.convention);
          }

          get_logger().logf(
            "Loaded material '{}'.",
            key);

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

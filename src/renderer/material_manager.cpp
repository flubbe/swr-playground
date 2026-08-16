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

#include "material_manager.h"
#include "render_device.h"
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

/*
 * MaterialEntry.
 */

MaterialEntry::~MaterialEntry()
{
    get_logger().logf(
      "Deleting material '{}'...",
      key);

    if(resolved_handle.has_value())
    {
        device.delete_material(
          resolved_handle.value());
    }

    material_manager.delete_material(key);
}

void MaterialEntry::finalize()
{
    if(resolved_handle.has_value())
    {
        return;
    }

    MaterialResources loaded = resources.future.get();

    Material material;

    bool success = false;
    auto rollback = gsl::finally(
      [&]()
      {
          if(!success)
          {
              name.clear();

              if(resolved_handle.has_value())
              {
                  device.delete_material(
                    resolved_handle.value());
              }

              // TODO Shader release is handled by the cache.

              base_color.reset();
              normal.reset();
          }
      });

    // FIXME Make name mandatory?
    name = loaded.description.name.value_or("");

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

        normal = texture_cache.load(
          generated_key,
          *loaded.normal_map);
        material.normal_map_handle = normal->get();
    }

    resolved_handle = device.create_material(material);
    success = true;
}

/*
 * MaterialManager.
 */

ResolvableMaterial MaterialManager::load(
  std::string_view path,
  std::string_view json)
{
    if(auto it = material_cache.find(path);
       it != material_cache.end())
    {
        return ResolvableMaterial{path, it->second};
    }

    // The material needs to be loaded. We delegate everything
    // to a task.

    auto submission = task_system.submit(
      [shader_factory = &shader_factory,
       json = swr::string{json},
       path = swr::string{path}](
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

          if(resources.description.normal.has_value())
          {
              if(context.is_cancel_requested())
              {
                  throw task_system::TaskCancelledError{};
              }

              const assets::NormalMapDesc& normal_map =
                *resources.description.normal;
              resources.normal_map = assets::load_normal_map_rgba8(
                normal_map.path,
                normal_map.convention);
          }

          get_logger().logf(
            "Loaded material '{}'.",
            path);

          // MaterialResources contains only CPU-side data and can be transferred
          // to the render/main thread for finalization.

          return resources;
      });

    auto material = std::make_shared<MaterialEntry>(
      *this,
      device,
      shader_cache,
      texture_cache,
      path,
      std::move(submission));

    material_cache.emplace(
      swr::string{path},
      material);

    // Push to pending material queue which is processed on render/main thread.
    pending_materials.emplace_back(
      std::make_pair(swr::string{path}, material));

    return ResolvableMaterial{path, material};
}

bool MaterialManager::delete_material(
  std::string_view path)
{
    return material_cache.erase(swr::string{path}) != 0;
}

void MaterialManager::process_pending()
{
    using namespace std::literals;

    // TODO Could make this subject to a time budget.
    auto material_queue = pending_materials.drain();
    for(auto& [key, entry]: material_queue)
    {
        if(entry->resources.future.wait_for(0ms) == std::future_status::ready)
        {
            entry->finalize();
            get_logger().logf(
              "Finalized material '{}'.",
              key);
        }
        else
        {
            // TODO We could place them into a temporary buffer and add them all at once.
            pending_materials.emplace_back(
              std::make_pair(
                std::move(key),
                std::move(entry)));
        }
    }
}

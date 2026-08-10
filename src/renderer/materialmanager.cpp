/**
 * Software Rasterizer Playground.
 *
 * Material management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "assets/material.h"
#include "materialmanager.h"
#include "renderdevice.h"
#include "shader_cache.h"
#include "shader_factory.h"
#include "texture_cache.h"

MaterialHandle MaterialManager::load(
  std::string_view key,
  std::string_view json)
{
    if(auto it = material_map.find(key);
       it != material_map.end())
    {
        return it->second;
    }

    auto desc = assets::load_material(json);

    auto shader = shader_cache.get(desc.shader);
    if(!shader.has_value())
    {
        auto shader_instance = shader_factory.get(desc.shader);
        if(!shader_instance)
        {
            throw std::runtime_error{
              std::format(
                "Cannot load material: Unknown shader '{}'.",
                desc.shader)};
        }

        shader = shader_cache.load(
          desc.shader,
          shader_instance);
    }

    swr::vector<TextureHandle> textures;
    textures.reserve(desc.textures.size());
    for(const auto& texture: desc.textures)
    {
        // look up by path.
        std::optional<TextureHandle> handle = texture_cache.get(texture.string());
        if(!handle.has_value())
        {
            auto image_data = assets::load_texture_rgba8(texture);
            textures.push_back(
              device.create_texture(image_data));

            // TODO handle normal maps (currently they have OpenGL or DirectX convention).
        }
    }

    auto material_handle = device.create_material(
      {.shader_handle = shader.value(),
       .texture_handles = std::move(textures)});

    // cache material.
    material_map.insert({swr::string{key}, material_handle});

    return material_handle;
}

bool MaterialManager::delete_material(
  std::string_view key)
{
    if(auto it = material_map.find(key);
       it != material_map.end())
    {
        device.delete_material(it->second);
        material_map.erase(it);
        return true;
    }

    return false;
}

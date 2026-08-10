/**
 * Software Rasterizer Playground.
 *
 * Texture cache.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "assets/material.h"
#include "renderer/renderdevice.h"
#include "shader_factory.h"
#include "texture_cache.h"

bool TextureCache::delete_texture(
  std::string_view key)
{
    if(auto it = texture_map.find(key);
       it != texture_map.end())
    {
        device.delete_texture(it->second);
        texture_map.erase(it);
        return true;
    }

    return false;
}

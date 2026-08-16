/**
 * Software Rasterizer Playground.
 *
 * Shader cache.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "renderer/render_device.h"
#include "shader_cache.h"

ShaderHandle ShaderCache::load(
  std::string_view key,
  const swr::program_base* shader)
{
    auto it = shader_map.find(key);
    if(it != shader_map.end())
    {
        return it->second;
    }

    auto shader_handle = device.create_shader(*shader);
    shader_map.insert({swr::string{key}, shader_handle});

    return shader_handle;
}

bool ShaderCache::delete_shader(
  std::string_view key)
{
    if(auto it = shader_map.find(key);
       it != shader_map.end())
    {
        device.delete_shader(it->second);
        shader_map.erase(it);
        return true;
    }

    return false;
}

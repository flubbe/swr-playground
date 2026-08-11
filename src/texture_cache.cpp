/**
 * Software Rasterizer Playground.
 *
 * Texture cache.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "renderer/renderdevice.h"
#include "texture_cache.h"

TextureEntry::~TextureEntry()
{
    device.delete_texture(handle);
}

TextureRef TextureCache::load(
  std::string_view key,
  const assets::ImageRGBA8& image)
{
    if(auto it = texture_map.find(key);
       it != texture_map.end())
    {
        return TextureRef{it->second};
    }

    auto entry = std::make_shared<TextureEntry>(
      device,
      device.create_texture(image));

    texture_map.emplace(
      swr::string{key},
      entry);

    return TextureRef{std::move(entry)};
}

bool TextureCache::delete_texture(
  std::string_view key)
{
    return texture_map.erase(swr::string{key}) != 0;
}

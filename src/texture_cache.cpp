/**
 * Software Rasterizer Playground.
 *
 * Texture cache.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "renderer/render_device.h"
#include "texture_cache.h"
#include "logging.h"

namespace
{
[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"TextureCache"};
    return logger;
}

}    // namespace

/*
 * TextureEntry.
 */

TextureEntry::~TextureEntry()
{
    device.delete_texture(handle);
}

/*
 * TextureCache.
 */

TextureRef TextureCache::load(
  std::string_view key,
  const assets::ImageRGBA8& image)
{
    if(auto it = texture_map.find(key);
       it != texture_map.end())
    {
        if(auto texture = it->second.lock())
        {
            get_logger().logf(
              "Using cached texture for '{}'.",
              key);

            return TextureRef{texture};
        }

        // Expired entry.
        get_logger().logf(
          "{} expired.",
          key);

        texture_map.erase(it);
    }

    get_logger().logf(
      "Creating texture for '{}'.",
      key);

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
    get_logger().logf(
      "Deleting texture for '{}'.",
      key);

    return texture_map.erase(swr::string{key}) != 0;
}

void TextureCache::prune()
{
    get_logger().logf("Pruning...");

    for(auto it = texture_map.begin(); it != texture_map.end();)
    {
        if(it->second.expired())
        {
            get_logger().logf(
              "'{}' expired.",
              it->first);

            it = texture_map.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

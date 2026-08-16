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
    if(auto it = texture_cache.find(key);
       it != texture_cache.end())
    {
        if(auto texture = it->second.lock())
        {
            get_logger().logf(
              "Using cached texture '{}'.",
              key);

            return TextureRef{texture};
        }

        // Expired entry.
        get_logger().logf(
          "Cached texture '{}' no longer exists; recreating.",
          key);

        texture_cache.erase(it);
    }
    else
    {
        get_logger().logf(
          "Creating texture '{}'.",
          key);
    }

    auto entry = std::make_shared<TextureEntry>(
      device,
      device.create_texture(image));

    texture_cache.emplace(
      swr::string{key},
      entry);

    return TextureRef{std::move(entry)};
}

bool TextureCache::delete_texture(
  std::string_view key)
{
    get_logger().logf(
      "Deleting texture '{}'.",
      key);

    return texture_cache.erase(swr::string{key}) != 0;
}

void TextureCache::prune()
{
    get_logger().logf("Pruning...");

    for(auto it = texture_cache.begin(); it != texture_cache.end();)
    {
        if(it->second.expired())
        {
            get_logger().logf(
              "Cache entry expired: '{}'",
              it->first);

            it = texture_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

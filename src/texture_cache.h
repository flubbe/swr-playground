/**
 * Software Rasterizer Playground.
 *
 * Texture cache.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "containers/format.h"

#include <xxhash.h>

#include "assets/texture.h"
#include "containers/string.h"
#include "containers/unordered_map.h"
#include "renderer/types.h"

/*
 * Forward declarations.
 */
class RenderDevice;

/** Texture cache. */
class TextureCache
{
    /** Render device reference. */
    RenderDevice& device;

    /** Cached textures. */
    swr::unordered_map<
      swr::string,
      TextureHandle>
      texture_map;

    /**
     * Hash image data.
     *
     * @param data The data to hash.
     * @returns Returns a 64-bit hash.
     */
    [[nodiscard]]
    static std::uint64_t compute_hash(
      const assets::ImageRGBA8& data) noexcept
    {
        return XXH3_64bits(data.pixels.data(), data.pixels.size());
    }

public:
    /**
     * Constructor.
     *
     * @param device The render device for this texture cache.
     */
    TextureCache(
      RenderDevice& device)
    : device{device}
    {
    }

    /**
     * Destructor. Releases all textures.
     *
     * @note Lifetime: The render device has to be valid.
     */
    ~TextureCache()
    {
        while(!texture_map.empty())
        {
            delete_texture(texture_map.begin()->first);
        }
    }

    /**
     * Load a texture and store it under a key.
     *
     * @note Deduplicates: When a key already exists, the corresponding
     *     texture handle is returned.
     *
     * @param key The texture key to use.
     * @param image The image data.
     * @returns Returns the texture handle.
     * @throws Throws a `std::runtime_error` if the load failed.
     */
    TextureHandle load(
      std::string_view key,
      const assets::ImageRGBA8& image);

    /**
     * Load a texture.
     *
     * @note Deduplicates: When the texture is already loaded, it's handle is returned.
     *
     * @param image The image data.
     * @returns Returns a pair `(texture_handle, key)`, where `key` can be used to
     *     access the texture in the manager.
     */
    std::pair<TextureHandle, swr::string> load(
      const assets::ImageRGBA8& image)
    {
        std::uint64_t hash = compute_hash(image);
        swr::string generated_key =
          swr::format("hash://{:016x}", hash);
        return std::make_pair(
          load(generated_key, image),
          std::move(generated_key));
    }

    /**
     * Get a texture by key.
     *
     * @param key The texture key.
     * @returns Returns the texture handle, or `std::nullopt` if the key wasn't found.
     */
    [[nodiscard]]
    std::optional<TextureHandle> get(
      std::string_view key) const
    {
        if(auto it = texture_map.find(key);
           it != texture_map.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * Check if the manager contains the texture.
     *
     * @param key The texture key.
     * @returns Returns `true` if the texture was found, and `false` otherwise.
     */
    [[nodiscard]]
    bool contains(
      std::string_view key) const
    {
        return texture_map.contains(key);
    }

    /**
     * Delete a texture by key.
     *
     * @param key Texture key.
     * @returns Returns `true` if the texture was deleted, and `false` if the key was not found.
     */
    bool delete_texture(
      std::string_view key);
};

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

/** A cached texture. */
struct TextureEntry
{
    /** Backing render device. */
    RenderDevice& device;

    /** GPU texture handle. */
    TextureHandle handle;

    /** Deleted default constructor. */
    TextureEntry() = delete;

    /**
     * Constructor.
     *
     * @param device The render device.
     * @param handle The texture handle.
     */
    TextureEntry(
      RenderDevice& device,
      TextureHandle handle)
    : device{device}
    , handle{handle}
    {
    }

    TextureEntry(const TextureEntry&) = delete;
    TextureEntry(TextureEntry&&) = delete;

    /** Destructor. */
    ~TextureEntry();
};

/** A reference to a cached texture. */
class TextureRef
{
    friend class TextureCache;

    /** Referenced texture entry. */
    swr::shared_ptr<TextureEntry> entry;

    /**
     * Constructor.
     *
     * @param entry The texture entry.
     */
    explicit TextureRef(
      swr::shared_ptr<TextureEntry> entry)
    : entry{std::move(entry)}
    {
    }

public:
    TextureRef() = delete;
    TextureRef(const TextureRef&) = default;
    TextureRef(TextureRef&&) = default;

    TextureRef& operator=(const TextureRef&) = default;
    TextureRef& operator=(TextureRef&&) = default;

    [[nodiscard]]
    TextureHandle get() const noexcept
    {
        return entry->handle;
    }

    [[nodiscard]]
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(entry);
    }
};

/** Texture cache. */
class TextureCache
{
    /** Render device reference. */
    RenderDevice& device;

    /** Cached textures. */
    swr::unordered_map<
      swr::string,
      swr::shared_ptr<TextureEntry>>
      texture_map;

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
        texture_map.clear();
    }

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
        return XXH3_64bits(
          data.pixels.data(),
          data.pixels.size());
    }

    /**
     * Load a texture and store it under a key.
     *
     * @note Deduplicates: When a key already exists, the corresponding
     *     texture reference is returned.
     *
     * @param key The texture key to use.
     * @param image The image data.
     * @returns Returns a reference to the texture.
     */
    TextureRef load(
      std::string_view key,
      const assets::ImageRGBA8& image);

    /**
     * Load a texture.
     *
     * @note Deduplicates: When the texture is already loaded, its reference
     *     is returned.
     *
     * @param image The image data.
     * @returns Returns a pair `(texture_reference, key)`.
     */
    std::pair<TextureRef, swr::string> load(
      const assets::ImageRGBA8& image)
    {
        const std::uint64_t hash = compute_hash(image);
        const swr::string generated_key =
          swr::format("hash://{:016x}", hash);

        return std::make_pair(
          load(generated_key, image),
          std::move(generated_key));
    }

    /**
     * Get a texture by key.
     *
     * @param key The texture key.
     * @returns Returns the texture reference, or `std::nullopt` if the key wasn't found.
     */
    [[nodiscard]]
    std::optional<TextureRef> get(
      std::string_view key) const
    {
        if(auto it = texture_map.find(key);
           it != texture_map.end())
        {
            return TextureRef{it->second};
        }

        return std::nullopt;
    }

    /**
     * Check if the cache contains the texture.
     *
     * @param key The texture key.
     * @returns Returns `true` if the texture was found.
     */
    [[nodiscard]]
    bool contains(
      std::string_view key) const
    {
        return texture_map.contains(key);
    }

    /**
     * Remove a texture from the cache.
     *
     * @note Existing references keep the texture alive.
     *
     * @param key Texture key.
     * @returns Returns `true` if the texture was removed.
     */
    bool delete_texture(
      std::string_view key);
};

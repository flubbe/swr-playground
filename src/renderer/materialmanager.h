/**
 * Software Rasterizer Playground.
 *
 * Material management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <utility>

#include <xxhash.h>

#include "containers/format.h"
#include "containers/unordered_map.h"
#include "material.h"

/*
 * Forward declarations.
 */

class RenderDevice;
class ShaderCache;
class ShaderFactory;
class TextureCache;

/** Material manager. */
class MaterialManager
{
    /** Render device reference. */
    RenderDevice& device;

    /** Shader cache. */
    ShaderCache& shader_cache;

    /** Shader factory. */
    ShaderFactory& shader_factory;

    /** Texture cache. */
    TextureCache& texture_cache;

    /** Material map. */
    swr::unordered_map<
      swr::string,
      MaterialHandle>
      material_map;

    /**
     * Hash a material string.
     *
     * @param data The data to hash.
     * @returns Returns a 64-bit hash.
     */
    [[nodiscard]]
    static std::uint64_t compute_hash(
      std::string_view data) noexcept
    {
        return XXH3_64bits(data.data(), data.size());
    }

public:
    /**
     * Constructor.
     *
     * @param device The render device for this material manager.
     */
    MaterialManager(
      RenderDevice& device,
      ShaderCache& shader_cache,
      ShaderFactory& shader_factory,
      TextureCache& texture_cache)
    : device{device}
    , shader_cache{shader_cache}
    , shader_factory{shader_factory}
    , texture_cache{texture_cache}
    {
    }

    /**
     * Destructor. Releases all materials.
     *
     * @note Lifetime: The render device has to be valid.
     */
    ~MaterialManager()
    {
        while(!material_map.empty())
        {
            delete_material(material_map.begin()->first);
        }
    }

    /**
     * Load a material from a JSON string and store it under a key.
     *
     * @note Deduplicates: When a key already exists, the corresponding
     *     material handle is returned.
     *
     * @param key The material key to use.
     * @param json The JSON string.
     * @returns Returns the material handle.
     * @throws Throws a `std::runtime_error` if the load failed.
     */
    MaterialHandle load(
      std::string_view key,
      std::string_view json);

    /**
     * Load a material from a JSON string.
     *
     * @note Deduplicates: When the material is already loaded, it's handle is returned.
     *
     * @param json The JSON string.
     * @returns Returns a pair `(material_handle, key)`, where `key` can be used to
     *     access the material in the manager.
     * @throws Throws a `std::runtime_error` if loading failed.
     */
    std::pair<MaterialHandle, swr::string> load(
      std::string_view json)
    {
        std::uint64_t hash = compute_hash(json);
        swr::string generated_key =
          swr::format("hash://{:016x}", hash);
        return std::make_pair(
          load(generated_key, json),
          std::move(generated_key));
    }

    /**
     * Get a material by key.
     *
     * @param key The material key.
     * @returns Returns the material handle, or `std::nullopt` if the key wasn't found.
     */
    [[nodiscard]]
    std::optional<MaterialHandle> get(
      std::string_view key) const
    {
        if(auto it = material_map.find(key);
           it != material_map.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * Check if the manager contains the material.
     *
     * @param key The material key.
     * @returns Returns `true` if the material was found, and `false` otherwise.
     */
    [[nodiscard]]
    bool contains(
      std::string_view key) const
    {
        return material_map.contains(key);
    }

    /**
     * Delete a material by key.
     *
     * @param key Material key.
     * @returns Returns `true` if the material was deleted, and `false` if the key was not found.
     */
    bool delete_material(
      std::string_view key);
};

/**
 * Software Rasterizer Playground.
 *
 * Shader cache.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "containers/format.h"
#include "containers/string.h"
#include "containers/unordered_map.h"
#include "renderer/types.h"

/*
 * Forward declarations.
 */
class RenderDevice;

/** Shader cache. */
class ShaderCache
{
    /** Render device reference. */
    RenderDevice& device;

    /** Cached shaders. */
    swr::unordered_map<
      swr::string,
      ShaderHandle>
      shader_map;

    /**
     * Hash a shader.
     *
     * @param shader The shader to hash.
     * @returns Returns a 64-bit hash.
     */
    [[nodiscard]]
    static std::uint64_t compute_hash(
      const swr::program_base* shader) noexcept
    {
        return std::hash<const swr::program_base*>{}(shader);
    }

public:
    /**
     * Constructor.
     *
     * @param device The render device for this shader cache.
     */
    ShaderCache(
      RenderDevice& device)
    : device{device}
    {
    }

    /**
     * Destructor. Releases all shaders.
     *
     * @note Lifetime: The render device has to be valid.
     */
    ~ShaderCache()
    {
        while(!shader_map.empty())
        {
            delete_shader(shader_map.begin()->first);
        }
    }

    /**
     * Load a shader and store it under a key.
     *
     * @note Deduplicates: When a key already exists, the corresponding
     *     shader handle is returned.
     *
     * @param key The shader key to use.
     * @param shader The shader.
     * @returns Returns the shader handle.
     * @throws Throws a `std::runtime_error` if the load failed.
     */
    ShaderHandle load(
      std::string_view key,
      const swr::program_base* shader);

    /**
     * Load a shader.
     *
     * @note Deduplicates: When the shader is already loaded, it's handle is returned.
     *
     * @param shader The shader.
     * @returns Returns a pair `(shader_handle, key)`, where `key` can be used to
     *     access the shader in the manager.
     */
    std::pair<ShaderHandle, swr::string> load(
      const swr::program_base* shader)
    {
        std::uint64_t hash = compute_hash(shader);
        swr::string generated_key =
          swr::format("hash://{:016x}", hash);
        return std::make_pair(
          load(generated_key, shader),
          std::move(generated_key));
    }

    /**
     * Get a shader by key.
     *
     * @param key The shader key.
     * @returns Returns the shader handle, or `std::nullopt` if the key wasn't found.
     */
    [[nodiscard]]
    std::optional<ShaderHandle> get(
      std::string_view key) const
    {
        if(auto it = shader_map.find(key);
           it != shader_map.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * Check if the manager contains the shader.
     *
     * @param key The shader key.
     * @returns Returns `true` if the shader was found, and `false` otherwise.
     */
    [[nodiscard]]
    bool contains(
      std::string_view key) const
    {
        return shader_map.contains(key);
    }

    /**
     * Delete a shader by key.
     *
     * @param key Shader key.
     * @returns Returns `true` if the shader was deleted, and `false` if the key was not found.
     */
    bool delete_shader(
      std::string_view key);
};

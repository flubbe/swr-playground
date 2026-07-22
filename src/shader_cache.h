#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>

#include <swr/shaders.h>

#include "containers/vector.h"

using ShaderCacheKey = void*;

/** Shader concept. */
template<typename T>
concept Shader = std::is_base_of_v<swr::program<T>, T>;

/**
 * Generate a unique tag per shader.
 *
 * @tparam T Shader class.
 */
template<Shader T>
ShaderCacheKey shader_cache_tag() noexcept
{
    /*
     * Uses the same unique-address tagging trick as the reflection system's
     * `reflect::detail::type_tag<T>()`: each template instantiation owns a
     * function-local static, and its address becomes the stable runtime tag.
     */

    static int tag = 0;
    return &tag;
}

/**
 * Shader cache. Manages shader creation.
 *
 * TODO Decide if we need targetted removal/invalidation.
 */
class ShaderCache
{
    /** Cached shaders. */
    swr::vector<
      std::unique_ptr<
        swr::program_base>>
      shaders;

    /** Shader keys for fast access. */
    std::unordered_map<
      ShaderCacheKey,
      swr::program_base*>
      shaders_by_key;

public:
    ShaderCache() = default;
    ShaderCache(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&&) = default;

    ~ShaderCache() = default;

    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache& operator=(ShaderCache&&) = default;

    /**
     * Checks if a shader exists in the cache and either returns it if found,
     * or creates a new shader.
     *
     * @tparam T Shader class.
     */
    template<Shader T>
    T* get_or_create()
    {
        const ShaderCacheKey key{shader_cache_tag<T>()};
        if(auto it = shaders_by_key.find(key); it != shaders_by_key.end())
        {
            return static_cast<T*>(it->second);
        }

        std::unique_ptr<T> new_shader = std::make_unique<T>();
        T* shader = new_shader.get();

        shaders.emplace_back(std::move(new_shader));
        shaders_by_key.emplace(key, shader);
        return shader;
    }

    /** Clear the cache. */
    void clear()
    {
        shaders.clear();
        shaders_by_key.clear();
    }

    /** Return the shader cache size (shader count). */
    std::size_t size() const
    {
        return shaders.size();
    }
};

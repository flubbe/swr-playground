#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <utility>

#include <ml/all.h>
#include <swr/swr.h>
#include <swr/shaders.h>

#include "containers/memory.h"
#include "containers/string.h"
#include "containers/unordered_map.h"
#include "containers/vector.h"

using ShaderCacheKey = void*;

/** Shader concept. */
template<typename T>
concept Shader = std::is_base_of_v<swr::program<T>, T>
                 && requires {
                        { T::name } -> std::convertible_to<std::string_view>;
                    };

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
      swr::unique_ptr<
        swr::program_base>>
      shaders;

    /** Shader keys for fast access. */
    swr::unordered_map<
      ShaderCacheKey,
      swr::program_base*>
      shaders_by_key;

    /** Shader names. */
    swr::unordered_map<
      swr::string,
      swr::program_base*>
      shaders_by_name;

public:
    ShaderCache() = default;
    ShaderCache(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&&) = default;

    ~ShaderCache() = default;

    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache& operator=(ShaderCache&&) = default;

    /**
     * Register a shader.
     *
     * @note Registration is idempotent.
     * @tparam T The shader to register.
     * @returns Returns `true` if the shader was newly registered, and `false`
     *     if it already existed.
     */
    template<Shader T>
    bool register_shader()
    {
        const ShaderCacheKey key{shader_cache_tag<T>()};
        if(auto it = shaders_by_key.find(key); it != shaders_by_key.end())
        {
            return false;
        }

        swr::unique_ptr<T> new_shader = swr::make_unique<T>();
        T* shader = new_shader.get();

        shaders.emplace_back(std::move(new_shader));
        shaders_by_key.emplace(key, shader);
        shaders_by_name.emplace(T::name, shader);

        return true;
    }

    /**
     * Checks if a shader exists in the cache and either returns it if found,
     * or creates a new shader.
     *
     * @tparam T Shader class.
     */
    template<Shader T>
    T* get_or_create()
    {
        register_shader<T>();

        const ShaderCacheKey key{shader_cache_tag<T>()};
        if(auto it = shaders_by_key.find(key); it != shaders_by_key.end())
        {
            return static_cast<T*>(it->second);
        }

        throw std::runtime_error{
          "Cannot find newly registered shader."};
    }

    /**
     * Get a shader by name.
     *
     * @param name The shader name.
     * @returns Returns the shader, or `nullptr` if not found.
     */
    swr::program_base* get(
      std::string_view name) const
    {
        auto it = shaders_by_name.find(swr::string{name});
        if(it == shaders_by_name.end())
        {
            return nullptr;
        }

        return it->second;
    }

    /** Get all shader names. */
    std::vector<swr::string> get_names() const
    {
        return shaders_by_name
               | std::views::transform(
                 [](const auto& s) -> swr::string
                 { return s.first; })
               | std::ranges::to<std::vector>();
    }

    /** Clear the cache. */
    void clear()
    {
        shaders.clear();
        shaders_by_key.clear();
        shaders_by_name.clear();
    }

    /** Return the shader cache size (shader count). */
    std::size_t size() const
    {
        return shaders.size();
    }
};

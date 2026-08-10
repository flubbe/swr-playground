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

#include <future>
#include <utility>

#include <xxhash.h>

#include "assets/material.h"
#include "assets/texture.h"
#include "containers/format.h"
#include "containers/unordered_map.h"
#include "containers/memory.h"
#include "material.h"
#include "tasks/task_system.h"

/*
 * Forward declarations.
 */

class RenderDevice;
class ShaderCache;
class ShaderFactory;
class TextureCache;

namespace swr
{
class program_base;
}    // namespace swr

/** Loaded material resources, as returned by the async loader. */
struct MaterialResources
{
    /** Material description loaded from disk. */
    assets::MaterialDesc description;

    /** Shader key. */
    swr::string shader_key;

    /** Shader, as loaded/resolved by the `ShaderFactory`. */
    const swr::program_base* shader;

    /** Texture data. */
    swr::vector<assets::ImageRGBA8> textures;
};

/** A material that is asynchronously resolved. */
class ResolvableMaterial
{
    struct State
    {
        /** Backing render device. */
        RenderDevice& render_device;

        /** Backing shader cache. */
        ShaderCache& shader_cache;

        /** Material resources future. */
        task_system::TaskSubmission<MaterialResources> resources;

        /** The handle returned once uploaded to the device. */
        std::optional<MaterialHandle> resolved_handle;
    };

    swr::shared_ptr<State> state;

public:
    ResolvableMaterial() = default;
    ResolvableMaterial(const ResolvableMaterial&) = default;
    ResolvableMaterial(ResolvableMaterial&&) = default;

    ResolvableMaterial& operator=(const ResolvableMaterial&) = default;
    ResolvableMaterial& operator=(ResolvableMaterial&&) = default;

    /**
     * Constructor.
     *
     * @param render_device The render device to use.
     * @param shader_cache The shader cache.
     * @param resources The resources future.
     */
    ResolvableMaterial(
      RenderDevice& render_device,
      ShaderCache& shader_cache,
      task_system::TaskSubmission<MaterialResources> resources)
    : state{std::make_shared<State>(
        State{
          .render_device = render_device,
          .shader_cache = shader_cache,
          .resources = std::move(resources),
          .resolved_handle = {}})}
    {
    }

    /** Checks if the material has finished uploading to the `RenderDevice`. */
    [[nodiscard]]
    bool is_resolved() const noexcept
    {
        if(!state)
        {
            return false;
        }

        return state->resolved_handle.has_value();
    }

    /**
     * Get the resolved material handle.
     *
     * @note Performs `RenderDevice` upload on first access.
     * @returns Returns the material handle.
     */
    MaterialHandle resolve();

    /** Checks if the underlying future is valid. */
    [[nodiscard]]
    bool valid() const
    {
        if(!state)
        {
            return false;
        }

        return state->resources.future.valid();
    }

    /** Blocks until the asynchronous resources have finished loading. */
    void wait()
    {
        if(!state)
        {
            throw std::logic_error{
              "Cannot wait on an invalid material."};
        }

        state->resources.future.wait();
    }

    /**
     * Waits for the asynchronous resources for up to the specified duration.
     *
     * @param timeout The maximum amount of time to wait.
     * @returns The status of the asynchronous operation.
     */
    template<typename Rep, typename Period>
    std::future_status wait_for(
      std::chrono::duration<Rep, Period> timeout)
    {
        if(!state)
        {
            throw std::logic_error{
              "Cannot wait on an invalid material."};
        }

        return state->resources.future.wait_for(timeout);
    }

    /**
     * Waits until the specified time point for the asynchronous resources.
     *
     * @param timeout_time The latest time to wait until.
     * @returns The status of the asynchronous operation.
     */
    template<typename Clock, typename Duration>
    std::future_status wait_until(
      std::chrono::time_point<Clock, Duration> timeout)
    {
        if(!state)
        {
            throw std::logic_error{
              "Cannot wait on an invalid material."};
        }

        return state->resources.future.wait_until(timeout);
    }
};

/** Material manager. */
class MaterialManager
{
    /** Task system for async loading. */
    task_system::TaskSystem& task_system;

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
      ResolvableMaterial>
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
      task_system::TaskSystem& task_system,
      RenderDevice& device,
      ShaderCache& shader_cache,
      ShaderFactory& shader_factory,
      TextureCache& texture_cache)
    : task_system{task_system}
    , device{device}
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
    ResolvableMaterial load(
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
    std::pair<ResolvableMaterial, swr::string> load(
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
    std::optional<ResolvableMaterial> get(
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

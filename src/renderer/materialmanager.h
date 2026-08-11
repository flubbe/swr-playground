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
#include "tasks/task_system.h"
#include "material.h"
#include "texture_cache.h"

/*
 * Forward declarations.
 */

class RenderDevice;
class ShaderCache;
class ShaderFactory;

namespace swr
{
class program_base;
}    // namespace swr

/** Loaded material resources, as returned by the async loader. */
struct MaterialResources
{
    /** Material description loaded from disk. */
    assets::MaterialDesc description;

    /** Shader, as loaded/resolved by the `ShaderFactory`. */
    const swr::program_base* shader;

    /** Texture data. */
    swr::vector<assets::ImageRGBA8> textures;
};

/** A material entry for a resolvable material. */
struct MaterialEntry
{
    /** Backing render device. */
    RenderDevice& device;

    /** Backing shader cache. */
    ShaderCache& shader_cache;

    /** Backing texture cache. */
    TextureCache& texture_cache;

    /** Material resources future. */
    task_system::TaskSubmission<MaterialResources> resources;

    /** Textures referenced by this material. */
    swr::vector<TextureRef> textures;

    /** The handle returned once uploaded to the device. */
    std::optional<MaterialHandle> resolved_handle;

    /** Deleted default constructor. */
    MaterialEntry() = delete;

    /**
     * Constructor.
     *
     * @param device Backing render device.
     * @param shader_cache Backing shader cache.
     * @param texture_cache Backing texture cache.
     * @param resources The resources future.
     */
    MaterialEntry(
      RenderDevice& device,
      ShaderCache& shader_cache,
      TextureCache& texture_cache,
      task_system::TaskSubmission<MaterialResources> resources)
    : device{device}
    , shader_cache{shader_cache}
    , texture_cache{texture_cache}
    , resources{std::move(resources)}
    , resolved_handle{std::nullopt}
    {
    }

    MaterialEntry(const MaterialEntry&) = delete;
    MaterialEntry(MaterialEntry&&) = delete;

    /** Destructor. */
    ~MaterialEntry();

    MaterialEntry& operator=(const MaterialEntry&) = delete;
    MaterialEntry& operator=(MaterialEntry&&) = delete;

    /** Checks if the material has finished uploading to the `RenderDevice`. */
    [[nodiscard]]
    bool is_resolved() const noexcept
    {
        return resolved_handle.has_value();
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
        return resources.future.valid();
    }

    /** Blocks until the asynchronous resources have finished loading. */
    void wait()
    {
        resources.future.wait();
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
        return resources.future.wait_for(timeout);
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
        return resources.future.wait_until(timeout);
    }
};

/** A material that is asynchronously resolved. */
class ResolvableMaterial
{
    friend class MaterialManager;

    /** The material entry, containing resources and handles. */
    swr::shared_ptr<MaterialEntry> entry;

    /**
     * Constructor.
     *
     * @param entry The material entry.
     */
    explicit ResolvableMaterial(
      swr::shared_ptr<MaterialEntry> entry)
    : entry{std::move(entry)}
    {
    }

public:
    /** Deleted default constructor. */
    ResolvableMaterial() = delete;

    /** Defaulted copy/moves. */
    ResolvableMaterial(const ResolvableMaterial&) = default;
    ResolvableMaterial(ResolvableMaterial&&) = default;

    ResolvableMaterial& operator=(const ResolvableMaterial&) = default;
    ResolvableMaterial& operator=(ResolvableMaterial&&) = default;

    MaterialEntry* operator->() noexcept
    {
        return entry.get();
    }

    const MaterialEntry* operator->() const noexcept
    {
        return entry.get();
    }

    MaterialEntry& operator*() noexcept
    {
        return *entry;
    }

    const MaterialEntry& operator*() const noexcept
    {
        return *entry;
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(entry);
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

    /** Material cache. */
    swr::unordered_map<
      swr::string,
      swr::shared_ptr<MaterialEntry>>
      material_cache;

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
        while(!material_cache.empty())
        {
            delete_material(material_cache.begin()->first);
        }
    }

    /**
     * Load a material from a JSON string and store it under a key.
     *
     * @note Deduplicates: When a key already exists, the corresponding
     *     material handle is returned.
     * @note The underlying `ShaderFactory` needs to stay alive while the material
     *     is asynchronously resolved.
     *
     * @param key The material key to use.
     * @param json The JSON string.
     * @returns Returns a resolvable material.
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
     * @returns Returns a resolvable material, or `std::nullopt` if the key wasn't found.
     */
    [[nodiscard]]
    std::optional<ResolvableMaterial> get(
      std::string_view key)
    {
        if(auto it = material_cache.find(key);
           it != material_cache.end())
        {
            return ResolvableMaterial{it->second};
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
        return material_cache.contains(key);
    }

    /**
     * Delete a material from the cache.
     *
     * @note This only affects the cache. Material handles that are still in use remain valid.
     * @param key Material key.
     * @returns Returns `true` if the material was deleted, and `false` if the key was not found.
     */
    bool delete_material(
      std::string_view key);
};

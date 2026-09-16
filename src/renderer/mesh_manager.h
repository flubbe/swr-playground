/**
 * Software Rasterizer Playground.
 *
 * Mesh management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <memory>

#include "assets/path.h"
#include "containers/unordered_map.h"
#include "mesh.h"
#include "queue.h"
#include "resource_tracker.h"

/*
 * Forward declarations.
 */

namespace task_system
{
class TaskSystem;
}    // namespace task_system

class MaterialRef;
class MeshEntry;
class RenderDevice;

/** Mesh manager. Handles loading and caching. */
class MeshManager
{
    /** Task system for async loading. */
    task_system::TaskSystem& task_system;

    /** Resource tracker. */
    ResourceTracker& resource_tracker;

    /** Render device reference. */
    RenderDevice& device;

    /** Pending mesh upload queue with entries `(key, mesh_entry)`. */
    ThreadSafeQueue<
      std::pair<
        assets::AssetPath,
        swr::shared_ptr<
          MeshEntry>>>
      pending_upload;

    /** Mesh cache. */
    swr::unordered_map<
      assets::AssetPath,
      std::weak_ptr<MeshEntry>>
      mesh_cache;

public:
    /** Explicitly disable copy construction. */
    MeshManager(const MeshManager&) = delete;

    /** Explicitly disable move constructor. */
    MeshManager(MeshManager&&) = delete;

    /**
     * Constructor.
     *
     * @param task_system The task system to use for async loading.
     * @param resource_tracker The resource tracker.
     * @param device The render device for this material manager.
     */
    MeshManager(
      task_system::TaskSystem& task_system,
      ResourceTracker& resource_tracker,
      RenderDevice& device)
    : task_system{task_system}
    , resource_tracker{resource_tracker}
    , device{device}
    {
    }

    /**
     * Default destructor.
     *
     * @note The render device has to be alive here, since queue or cache entries
     *     might get released.
     */
    ~MeshManager() = default;

    /**
     * Schedule mesh loading.
     *
     * @param path Mesh asset path.
     * @param material Material reference.
     * @returns Returns a mesh reference.
     */
    MeshRef load(
      const assets::AssetPath& path,
      MaterialRef& material);

    /**
     * Get a cached mesh reference.
     *
     * @param path Mesh asset path.
     * @returns Returns a mesh reference if found in the cache,
     *     and `std::nullopt` otherwise.
     */
    [[nodiscard]]
    std::optional<MeshRef> try_get(
      const assets::AssetPath& path);

    /**
     * Delete a mesh.
     *
     * @note Not implemented yet.
     */
    bool delete_mesh(
      const assets::AssetPath& path);

    /**
     * Process pending meshes.
     *
     * @note Needs to be called from the render/main thread.
     */
    void process_pending();

    /** Remove expired cache entries. */
    void prune();

    /** Return the number of mesh cache entries. */
    [[nodiscard]]
    std::size_t cache_size() const noexcept
    {
        return mesh_cache.size();
    }

    /** Return the number of live mesh cache entries. */
    [[nodiscard]]
    std::size_t live_cache_size() const
    {
        return std::ranges::count_if(
          mesh_cache,
          [](const auto& entry)
          { return !entry.second.expired(); });
    }

    /** Return the number of pending mesh uploads. */
    [[nodiscard]]
    std::size_t pending_count() const
    {
        return pending_upload.size();
    }
};

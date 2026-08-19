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

#include <memory>

#include "assets/path.h"
#include "containers/unordered_map.h"
#include "mesh.h"
#include "queue.h"

/*
 * Forward declarations.
 */

namespace task_system
{
class TaskSystem;
}    // namespace task_system

class RenderDevice;

class MeshManager
{
    /** Task system for async loading. */
    task_system::TaskSystem& task_system;

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
    /**
     * Constructor.
     *
     * @param task_system The task system to use for async loading.
     * @param device The render device for this material manager.
     */
    MeshManager(
      task_system::TaskSystem& task_system,
      RenderDevice& device)
    : task_system{task_system}
    , device{device}
    {
    }

    // TODO Constructor.
    // TODO Destructor.

    MeshRef load(
      const assets::AssetPath& path,
      MaterialRef& material);

    [[nodiscard]]
    std::optional<MeshRef> get(
      const assets::AssetPath& path);

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
};

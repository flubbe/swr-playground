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

/*
 * Forward declarations.
 */

namespace task_system
{
class TaskSystem;
}    // namespace task_system

/** Mesh cache entry. */
struct MeshCacheEntry
{
    swr::vector<std::weak_ptr<MeshLodEntry>> lods;
};

class MeshManager
{

    /** Task system for async loading. */
    task_system::TaskSystem& task_system;

    /** Mesh cache. */
    swr::unordered_map<
      assets::AssetPath,
      MeshCacheEntry>
      mesh_cache;

public:
    /**
     * Constructor.
     *
     * @param task_system The task system to use for async loading.
     */
    MeshManager(
      task_system::TaskSystem& task_system)
    : task_system{task_system}
    {
    }

    // TODO Constructor.
    // TODO Destructor.

    MeshRef load(
      const assets::AssetPath& path);

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

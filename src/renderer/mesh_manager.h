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

#include "mesh.h"

class MeshManager
{
public:
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

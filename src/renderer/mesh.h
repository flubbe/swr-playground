/**
 * Software Rasterizer Playground.
 *
 * A mesh that is potentially asynchronously resolved.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "assets/path.h"
#include "meshes/mesh.h"
#include "tasks/task_system.h"
#include "types.h"

/*
 * Forward declarations.
 */

class MeshEntry;
class RenderDevice;

/** A mesh that is asynchronously loaded. */
class MeshRef
{
    /** Mesh asset path. */
    assets::AssetPath path;

    /** Mesh. */
    swr::shared_ptr<MeshEntry> mesh;

public:
    /** Deleted default constructor. */
    MeshRef() = delete;

    /** Defaulted copy/moves. */
    MeshRef(const MeshRef&) = default;
    MeshRef(MeshRef&&) = default;

    /**
     * Constructor.
     *
     * @param path Path identifying the mesh.
     * @param entry The mesh entry.
     */
    explicit MeshRef(
      const assets::AssetPath& path,
      swr::shared_ptr<MeshEntry> entry)
    : path{path}
    , mesh{std::move(entry)}
    {
    }

    MeshRef& operator=(const MeshRef&) = default;
    MeshRef& operator=(MeshRef&&) = default;

    [[nodiscard]]
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(mesh);
    }

    /** Get the mesh LOD handles. */
    [[nodiscard]]
    const std::vector<MeshHandle>&
      try_get() const noexcept;

    /** Get the path identifying this mesh. */
    const assets::AssetPath& get_path() const
    {
        return path;
    }

    /** Get the `MeshEntry`. */
    [[nodiscard]]
    MeshEntry& get_entry()
    {
        return *mesh;
    }

    /** Get the `MeshEntry`. */
    [[nodiscard]]
    const MeshEntry& get_entry() const
    {
        return *mesh;
    }
};

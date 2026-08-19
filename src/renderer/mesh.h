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
#include "staged_data.h"
#include "types.h"

/*
 * Forward declarations.
 */

class RenderDevice;

class MeshEntry
{
    friend class MeshManager;

    /** Backing render device. */
    RenderDevice& device;

    /** Mesh material. */
    MaterialRef material;

    /** CPU mesh data. deleted after driver upload. */
    task_system::TaskSubmission<StagedStaticMeshAsset> resources;

    /** Mesh LOD handles. */
    std::optional<
      std::vector<MeshHandle>>
      resolved_handles;

public:
    /** Deleted default constructor. */
    MeshEntry() = delete;

    /**
     * Constructor.
     *
     * @param device Backing render device.
     * @param material Mesh material reference.
     * @param resources The resources task submission.
     */
    MeshEntry(
      RenderDevice& device,
      MaterialRef& material,
      task_system::TaskSubmission<StagedStaticMeshAsset> resources)
    : device{device}
    , material{material}
    , resources{std::move(resources)}
    , resolved_handles{std::nullopt}
    {
    }

    /** Destructor. */
    ~MeshEntry();

    MeshEntry& operator=(const MeshEntry&) = delete;
    MeshEntry& operator=(MeshEntry&&) = delete;

    /** Checks if the mesh has finished uploading to the `RenderDevice`, including LODs. */
    [[nodiscard]]
    bool is_resolved() const noexcept
    {
        return resolved_handles.has_value();
    }

    /**
     * Get the mesh handle if the mesh is resolved.
     *
     * @returns Returns the mesh handle if available, or `std::nullopt`.
     */
    const std::optional<
      std::vector<MeshHandle>>&
      try_get() const noexcept
    {
        return resolved_handles;
    }

    /**
     * Finalize mesh loading.
     *
     * @note Performs `RenderDevice` access and needs to be called from the render thread.
     */
    void finalize();

    /**
     * Destroy the mesh handle.
     *
     * @note Performs `RenderDevice` access and needs to be called from the render thread.
     */
    void release();

    /** Checks if the underlying future is valid. */
    [[nodiscard]]
    bool valid() const
    {
        return resources.future.valid();
    }

    /** Blocks until the asynchronous resources have finished loading. */
    void wait()
    {
        if(valid())
        {
            resources.future.wait();
        }
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

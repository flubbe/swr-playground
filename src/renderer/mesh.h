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

class RenderDevice;

class MeshLodEntry
{
    task_system::TaskSubmission<MeshData> resources;    // CPU mesh data. deleted after driver upload.
    std::optional<MeshHandle> resolved_handle;          // mesh section handle

public:
    MeshLodEntry() = delete;

    // TODO constructors.

    MeshLodEntry& operator=(const MeshLodEntry&) = delete;
    MeshLodEntry& operator=(MeshLodEntry&&) = delete;

    /** Checks if the mesh has finished uploading to the `RenderDevice`. */
    [[nodiscard]]
    bool is_resolved() const noexcept
    {
        return resolved_handle.has_value();
    }

    /**
     * Get the mesh handle if the mesh is resolved.
     *
     * @returns Returns the mesh handle if available, or `std::nullopt`.
     */
    std::optional<MeshHandle> try_get() const noexcept
    {
        return resolved_handle;
    }

    /**
     * Finalize mesh loading.
     *
     * @note Performs `RenderDevice` upload and needs to be called from the render thread.
     * @param render_device The device use for upload.
     */
    void finalize(
      RenderDevice& render_device);

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

/** A reference to a mesh LOD. */
struct MeshLodRef
{
    /** Referenced mesh lod entry. */
    swr::shared_ptr<MeshLodEntry> entry;

public:
    MeshLodRef() = delete;
    MeshLodRef(const MeshLodRef&) = default;
    MeshLodRef(MeshLodRef&&) = default;

    /**
     * Constructor.
     *
     * @param entry The mesh entry.
     */
    explicit MeshLodRef(
      swr::shared_ptr<MeshLodEntry> entry)
    : entry{std::move(entry)}
    {
    }

    MeshLodRef& operator=(const MeshLodRef&) = default;
    MeshLodRef& operator=(MeshLodRef&&) = default;

    [[nodiscard]]
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(entry);
    }

    /** Get the mesh handle. */
    [[nodiscard]]
    std::optional<MeshHandle> try_get() const noexcept;

    /** Get the `MeshLodEntry`. */
    [[nodiscard]]
    MeshLodEntry& get_entry() noexcept
    {
        return *entry.get();
    }

    /** Get the `MeshLodEntry`. */
    [[nodiscard]]
    const MeshLodEntry& get_entry() const noexcept
    {
        return *entry.get();
    }
};

// TODO Create lazily resolved mesh reference.
class MeshRef
{
    assets::AssetPath path;
    swr::vector<MeshLodRef> lods;

public:
    std::size_t lod_count() const
    {
        return lods.size();
    }

    MeshLodRef lod(std::size_t index) const
    {
        return lods.at(index);    // FIXME Should this throw?
    }
};

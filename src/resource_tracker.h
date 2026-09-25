/**
 * Software Rasterizer Playground.
 *
 * A thread safe resource tracker.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <mutex>
#include <optional>
#include <unordered_map>

#include "assets/path.h"

/*
 * Forward declarations.
 */

class ResourceTracker;

/** Resource id. */
struct ResourceId
{
    using Type = std::size_t;
    Type value;

    bool operator==(const ResourceId& other) const noexcept = default;
};

namespace std
{

template<>
struct hash<ResourceId>
{
    std::size_t operator()(const ResourceId& id) const noexcept
    {
        return std::hash<decltype(ResourceId::value)>{}(id.value);
    }
};

}    // namespace std

/** Resource state. */
enum class ResourceState
{
    Pending,   /** Load is pending. */
    Completed, /** Loading completed. */
    Failed,    /** Loading failed. */
    Cancelled  /* Loading was cancelled. */
};

struct ResourceRecord
{
    ResourceState state;
    std::optional<assets::AssetPath> path;
};

class ResourceTicket
{
    friend ResourceTracker;

    ResourceTracker& tracker;
    ResourceId id;

    ResourceTicket(
      ResourceTracker& tracker,
      ResourceId id)
    : tracker{tracker}
    , id{id}
    {
    }

public:
    ResourceTicket() = delete;
    ResourceTicket(const ResourceTicket&) = default;
    ResourceTicket(ResourceTicket&&) noexcept = default;

    /** Return the associated tracker. */
    ResourceTracker& get_tracker() noexcept
    {
        return tracker;
    }

    /** Return the associated tracker. */
    const ResourceTracker& get_tracker() const noexcept
    {
        return tracker;
    }

    /** Get the resource id. */
    ResourceId get_id() const noexcept
    {
        return id;
    }

    /** Get the current state. */
    ResourceState get_state() const;

    /** Set the state to pending. */
    void pending();

    /** Set the state to completed. */
    void completed();

    /** Set the state to failed. */
    void failed();

    /** Set the state to cancelled. */
    void cancelled();
};

/** Keeps track of resource loading state. */
class ResourceTracker
{
    friend ResourceTicket;

    /** Tracker mutex. */
    mutable std::mutex mutex;

    /** Tracked resources. */
    std::unordered_map<
      ResourceId,
      ResourceRecord>
      resources;

    /** Next available resourced id. */
    ResourceId::Type next_id = 0;

public:
    /** Default constructor. */
    ResourceTracker() = default;

    /** Disable copy/move. */
    ResourceTracker(const ResourceTracker&) = delete;
    ResourceTracker(ResourceTracker&&) = delete;

    /**
     * Track a new resource.
     *
     * @returns Returns a `ResourceTicket` for tracking.
     */
    ResourceTicket track();

    /**
     * Track a new resource.
     *
     * @param path The resource path.
     * @returns Returns a `ResourceTicket` for tracking.
     */
    ResourceTicket track(
      const assets::AssetPath& path);

    /** Get the number of tracked resources. */
    std::size_t size() const
    {
        std::unique_lock lock{mutex};
        return resources.size();
    }

    /** Get whether no resources are tracked. */
    bool empty() const
    {
        return size() == 0;
    }

    /** Get the number of pending resources. */
    std::size_t pending_count() const;

    /** Get the number of completed resources. */
    std::size_t completed_count() const;

    /** Get the number of failed resources. */
    std::size_t failed_count() const;

    /** Get the number of cancelled resources. */
    std::size_t cancelled_count() const;

    /** Get whether all resources have completed successfully. */
    bool is_complete() const;

    /** Get whether all resources have finished, successfully or otherwise. */
    bool is_finished() const;

    /** Get whether any resource has failed. */
    bool has_failed() const;

    /** Clear all resources. */
    void clear();

    /** Clear all finished resources. */
    void clear_finished();
};

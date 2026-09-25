#include <limits>
#include <ranges>
#include <stdexcept>

#include "resource_tracker.h"

/*
 * ResourceTicket.
 */

ResourceState ResourceTicket::get_state() const
{
    std::unique_lock lock{tracker.mutex};

    auto it = tracker.resources.find(id);
    if(it == tracker.resources.end())
    {
        throw std::runtime_error{
          "ResourceTicket refers to an invalid resource"};
    }

    return it->second.state;
}

namespace
{

void set_state(
  ResourceId id,
  std::unordered_map<ResourceId, ResourceRecord>& records,
  ResourceState state)
{
    auto it = records.find(id);
    if(it == records.end())
    {
        throw std::runtime_error{
          "ResourceTicket refers to an invalid resource"};
    }

    it->second.state = state;
}

}    // namespace

void ResourceTicket::pending()
{
    std::unique_lock lock{tracker.mutex};
    set_state(
      id,
      tracker.resources,
      ResourceState::Pending);
}

void ResourceTicket::completed()
{
    std::unique_lock lock{tracker.mutex};
    set_state(
      id,
      tracker.resources,
      ResourceState::Completed);
}

void ResourceTicket::failed()
{
    std::unique_lock lock{tracker.mutex};
    set_state(
      id,
      tracker.resources,
      ResourceState::Failed);
}

void ResourceTicket::cancelled()
{
    std::unique_lock lock{tracker.mutex};
    set_state(
      id,
      tracker.resources,
      ResourceState::Cancelled);
}

/*
 * ResourceTracker.
 */

ResourceTicket ResourceTracker::track()
{
    std::unique_lock lock{mutex};

    if(next_id == std::numeric_limits<ResourceId::Type>::max())
    {
        throw std::overflow_error("ResourceTracker ID exhausted");
    }

    auto new_id = ResourceId{next_id++};

    resources.emplace(
      new_id,
      ResourceRecord{
        .state = ResourceState::Pending,
        .path = std::nullopt});

    return {*this, new_id};
}

ResourceTicket ResourceTracker::track(
  const assets::AssetPath& path)
{
    std::unique_lock lock{mutex};

    if(next_id == std::numeric_limits<ResourceId::Type>::max())
    {
        throw std::overflow_error("ResourceTracker ID exhausted");
    }

    auto new_id = ResourceId{next_id++};

    resources.emplace(
      new_id,
      ResourceRecord{
        .state = ResourceState::Pending,
        .path = path});

    return {*this, new_id};
}

namespace
{

/** Count the records having a target state. */
std::size_t count_state(
  const std::unordered_map<
    ResourceId,
    ResourceRecord>& records,
  ResourceState target)
{
    return std::ranges::count_if(
      records,
      [target](const auto& p) -> bool
      { return p.second.state == target; });
}

}    // namespace

std::size_t ResourceTracker::pending_count() const
{
    std::unique_lock lock{mutex};
    return count_state(
      resources,
      ResourceState::Pending);
}

std::size_t ResourceTracker::completed_count() const
{
    std::unique_lock lock{mutex};
    return count_state(
      resources,
      ResourceState::Completed);
}

std::size_t ResourceTracker::failed_count() const
{
    std::unique_lock lock{mutex};
    return count_state(
      resources,
      ResourceState::Failed);
}

std::size_t ResourceTracker::cancelled_count() const
{
    std::unique_lock lock{mutex};
    return count_state(
      resources,
      ResourceState::Cancelled);
}

bool ResourceTracker::is_complete() const
{
    std::unique_lock lock{mutex};
    return std::ranges::all_of(
      resources,
      [](const auto& p)
      {
          return p.second.state == ResourceState::Completed;
      });
}

bool ResourceTracker::is_finished() const
{
    std::unique_lock lock{mutex};
    return std::ranges::none_of(
      resources,
      [](const auto& p) -> bool
      { return p.second.state == ResourceState::Pending; });
}

bool ResourceTracker::has_failed() const
{
    std::unique_lock lock{mutex};
    return std::ranges::any_of(
      resources,
      [](const auto& p) -> bool
      { return p.second.state == ResourceState::Failed; });
}

void ResourceTracker::clear()
{
    std::unique_lock lock{mutex};
    resources.clear();
}

void ResourceTracker::clear_finished()
{
    std::unique_lock lock{mutex};
    std::erase_if(
      resources,
      [](const auto& p) -> bool
      {
          return p.second.state != ResourceState::Pending;
      });
}

/**
 * Software Rasterizer Playground.
 *
 * Mesh management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <gsl/gsl>

#include "assets/path_formatter.h"
#include "assets/static_mesh_importer.h"
#include "scene/static_mesh.h"
#include "meshes/lod.h"
#include "colors.h"
#include "logging.h"
#include "material.h"
#include "mesh_manager.h"
#include "render_device.h"
#include "staged_data.h"

namespace
{

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"MeshManager"};
    return logger;
}

swr::vector<staged::StaticMeshSection> build_static_mesh_sections(
  ImportedStaticMesh imported_mesh)
{
    const StaticMeshLodBuildSettings lod_settings{
      .preserve_boundaries = false,
      .recompute_normals = true,
    };

    swr::vector<staged::StaticMeshSection> sections;
    sections.reserve(imported_mesh.meshes.size());

    for(auto& mesh: imported_mesh.meshes)
    {
        const StaticMeshLodBuildResult lod_build_result =
          build_static_mesh_lods(
            mesh.mesh_data,
            lod_settings);

        staged::StaticMeshSection section{
          .diffuse_color = mesh.diffuse_color,
          .lods = {}};
        section.lods.reserve(lod_build_result.lod_meshes.size());

        for(const auto& lod_mesh: lod_build_result.lod_meshes)
        {
            section.lods.push_back(
              staged::StaticMeshSectionLod{
                .mesh = lod_mesh,
                .bounds = calculate_mesh_bounds(lod_mesh),
              });
        }

        sections.push_back(std::move(section));
    }

    return sections;
}

}    // namespace

/*
 * MeshEntry.
 */

class MeshEntry
{
    friend class MeshManager;

    /** Backing render device. */
    RenderDevice& device;

    /** Mesh material. */
    MaterialRef material;

    /** CPU mesh data. deleted after driver upload. */
    task_system::TaskSubmission<
      staged::StaticMeshAsset>
      resources;

    /** Resolved mesh sections and their GPU handles. */
    std::optional<
      swr::vector<MeshSection>>
      resolved_sections;

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
      const MaterialRef& material,
      task_system::TaskSubmission<
        staged::StaticMeshAsset>
        resources)
    : device{device}
    , material{material}
    , resources{std::move(resources)}
    , resolved_sections{std::nullopt}
    {
    }

    /** Construct an already uploaded mesh entry. */
    MeshEntry(
      RenderDevice& device,
      const MaterialRef& material,
      staged::StaticMeshAsset loaded)
    : device{device}
    , material{material}
    , resources{}
    , resolved_sections{std::nullopt}
    {
        finalize(std::move(loaded));
    }

    /** Destructor. */
    ~MeshEntry();

    MeshEntry& operator=(const MeshEntry&) = delete;
    MeshEntry& operator=(MeshEntry&&) = delete;

    /** Checks if the mesh has finished uploading to the `RenderDevice`, including LODs. */
    [[nodiscard]]
    bool is_resolved() const noexcept
    {
        return resolved_sections.has_value();
    }

    /**
     * Get the mesh LODs if the mesh is resolved.
     *
     * @returns Returns the mesh LODs if available, or `std::nullopt`.
     */
    const std::optional<
      swr::vector<MeshSection>>&
      try_get_sections() const noexcept
    {
        return resolved_sections;
    }

    /**
     * Finalize mesh loading.
     *
     * @note Performs `RenderDevice` access and needs to be called from the render thread.
     */
    void finalize();

    /**
     * Finalize mesh loading.
     *
     * @param mesh The mesh asset to finalize.
     * @note Performs `RenderDevice` access and needs to be called from the render thread.
     */
    void finalize(
      staged::StaticMeshAsset mesh);

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

    /** Cancel and wait for the asynchronous load. */
    void cancel_and_wait()
    {
        resources.handle.cancel();
        wait();
    }

    /** Request cancellation of an asynchronous load. */
    void cancel()
    {
        resources.handle.cancel();
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

MeshEntry::~MeshEntry()
{
    if(resolved_sections.has_value())
    {
        for(const auto& section: resolved_sections.value())
        {
            for(const auto& lod: section.lods)
            {
                device.defer_delete(lod.mesh_handle);
            }
        }
    }
}

void MeshEntry::finalize()
{
    finalize(resources.future.get());
}

void MeshEntry::finalize(
  staged::StaticMeshAsset mesh)
{
    if(is_resolved())
    {
        return;
    }

    if(mesh.sections.empty())
    {
        throw std::runtime_error{
          "Mesh asset contains no renderable sections."};
    }

    swr::vector<MeshSection> result_sections;

    bool success = false;
    auto rollback = gsl::finally(
      [&]()
      {
          if(!success)
          {
              for(auto& section: result_sections)
              {
                  for(auto& lod: section.lods)
                  {
                      device.delete_mesh(
                        lod.mesh_handle);
                  }
              }
          }
      });

    const auto section_count = mesh.sections.size();
    result_sections.reserve(section_count);

    // TODO Bounds calculation could happen during load.

    for(std::size_t i = 0; i < section_count; ++i)
    {
        const staged::StaticMeshSection& section = mesh.sections[i];

        auto mesh_section = MeshSection{
          .color = section.diffuse_color,
          .lods = {},
          .material = material,
          .bounds = {}};

        for(std::size_t lod_index = 0;
            lod_index < section.lods.size();
            ++lod_index)
        {
            const staged::StaticMeshSectionLod& staged_lod =
              section.lods[lod_index];

            // TODO Don't accumulate?
            expand_bounds(
              mesh_section.bounds,
              staged_lod.bounds);

            const MeshHandle mesh_handle = device.create_mesh(staged_lod.mesh);
            mesh_section.lods.emplace_back(
              SectionLOD{
                .mesh_handle = mesh_handle,
                .triangle_count = staged_lod.mesh.indices.size() / 3});
        }

        result_sections.push_back(mesh_section);
    }

    resolved_sections = std::move(result_sections);
    success = true;
}

void MeshEntry::release()
{
    if(!is_resolved())
    {
        return;
    }

    for(auto& section: resolved_sections.value())
    {
        for(auto& lod: section.lods)
        {
            device.delete_mesh(lod.mesh_handle);
        }
    }

    resolved_sections.reset();
}

/*
 * MeshRef.
 */

const swr::vector<MeshSection>*
  MeshRef::try_get_sections() const noexcept
{
    if(!mesh)
    {
        return nullptr;
    }

    const auto& sections = mesh->try_get_sections();
    return sections.has_value()
             ? &sections.value()
             : nullptr;
}

/*
 * MeshManager.
 */

MeshManager::~MeshManager()
{
    clear();
}

MeshRef MeshManager::load(
  const assets::AssetPath& path,
  MaterialRef& material)
{
    // Collect cached LOD's.
    auto cached_mesh = try_get(path);
    if(cached_mesh.has_value())
    {
        get_logger().logf(
          "Using cached mesh '{}'.",
          path);

        return cached_mesh.value();
    }

    auto resource_ticket = resource_tracker.track(path);

    get_logger().logf(
      "Loading mesh '{}'.",
      path);

    // The mesh needs to be loaded. We delegate everything
    // to a task.

    auto submission = task_system.submit(
      [resource_ticket,
       path = assets::AssetPath{path}](
        task_system::TaskExecutionContext& context) mutable -> staged::StaticMeshAsset
      {
          if(context.is_cancel_requested())
          {
              resource_ticket.cancelled();
              throw task_system::TaskCancelledError{};
          }

          auto imported_mesh = import_static_mesh(path.path);

          if(context.is_cancel_requested())
          {
              resource_ticket.cancelled();
              throw task_system::TaskCancelledError{};
          }

          // TODO Fix color space. This works for some models.
          for(auto& mesh: imported_mesh.meshes)
          {
              mesh.diffuse_color = colors::linear_to_srgb(mesh.diffuse_color);
          }

          auto sections = build_static_mesh_sections(std::move(imported_mesh));
          std::erase_if(
            sections,
            [](const staged::StaticMeshSection& section)
            {
                return section.lods.empty();
            });

          get_logger().logf(
            "Loaded mesh '{}'.",
            path);

          resource_ticket.completed();

          // staged::StaticMeshAsset contains only CPU-side data and can be transferred
          // to the render/main thread for finalization.

          return staged::StaticMeshAsset{
            .path = path,
            .sections = std::move(sections),
          };
      });

    auto mesh = std::make_shared<MeshEntry>(
      device,
      material,
      std::move(submission));

    mesh_cache.emplace(
      path,
      mesh);

    // Push to pending mesh queue which is processed on render/main thread.
    pending_upload.emplace_back(
      std::make_pair(path, mesh));

    return MeshRef{
      path,
      mesh};
}

MeshRef MeshManager::load(
  const assets::AssetPath& path,
  std::vector<MeshData> data,
  const MaterialRef& material)
{
    // Collect cached LOD's.
    auto cached_mesh = try_get(path);
    if(cached_mesh.has_value())
    {
        get_logger().logf(
          "Using cached mesh '{}'.",
          path);

        return cached_mesh.value();
    }

    auto resource_ticket = resource_tracker.track(path);

    get_logger().logf(
      "Loading mesh '{}'.",
      path);

    auto submission = task_system.submit(
      [resource_ticket,
       data = std::move(data),
       path = assets::AssetPath{path}](
        task_system::TaskExecutionContext& context) mutable -> staged::StaticMeshAsset
      {
          if(context.is_cancel_requested())
          {
              resource_ticket.cancelled();
              throw task_system::TaskCancelledError{};
          }

          ImportedStaticMesh imported_mesh;
          imported_mesh.meshes.reserve(data.size());
          for(auto& mesh_data: data)
          {
              imported_mesh.meshes.push_back(
                ImportedMesh{
                  .name = {},
                  .mesh_data = std::move(mesh_data),
                  .diffuse_color = {0.8f, 0.8f, 0.8f, 1.0f},
                  .bounds = {}});
          }

          auto sections = build_static_mesh_sections(std::move(imported_mesh));
          std::erase_if(
            sections,
            [](const staged::StaticMeshSection& section)
            {
                return section.lods.empty();
            });

          if(context.is_cancel_requested())
          {
              resource_ticket.cancelled();
              throw task_system::TaskCancelledError{};
          }

          get_logger().logf(
            "Loaded mesh '{}'.",
            path);

          resource_ticket.completed();

          return staged::StaticMeshAsset{
            .path = path,
            .sections = std::move(sections),
          };
      });

    auto mesh = std::make_shared<MeshEntry>(
      device,
      material,
      std::move(submission));

    mesh_cache.emplace(
      path,
      mesh);

    pending_upload.emplace_back(
      std::make_pair(path, mesh));

    return MeshRef{
      path,
      mesh};
}

MeshRef MeshManager::reload_async(
  const assets::AssetPath& path,
  std::vector<MeshData> data,
  const MaterialRef& material)
{
    // Cancel pending load.
    if(auto it = mesh_cache.find(path);
       it != mesh_cache.end())
    {
        if(auto mesh = it->second.lock();
           mesh
           && !mesh->is_resolved())
        {
            mesh->cancel();
        }

        mesh_cache.erase(it);
    }

    return load(
      path,
      std::move(data),
      material);
}

MeshRef MeshManager::reload_sync(
  const assets::AssetPath& path,
  std::vector<MeshData> data,
  const MaterialRef& material)
{
    // Cancel pending load.
    if(auto it = mesh_cache.find(path);
       it != mesh_cache.end())
    {
        if(auto mesh = it->second.lock();
           mesh
           && !mesh->is_resolved())
        {
            mesh->cancel_and_wait();
        }

        mesh_cache.erase(it);
    }

    ImportedStaticMesh imported_mesh;
    imported_mesh.meshes.reserve(data.size());
    for(auto& mesh_data: data)
    {
        imported_mesh.meshes.push_back(
          ImportedMesh{
            .name = {},
            .mesh_data = std::move(mesh_data),
            .diffuse_color = {0.8f, 0.8f, 0.8f, 1.0f},
            .bounds = {}});
    }

    auto sections = build_static_mesh_sections(std::move(imported_mesh));
    std::erase_if(
      sections,
      [](const staged::StaticMeshSection& section)
      {
          return section.lods.empty();
      });

    auto mesh = std::make_shared<MeshEntry>(
      device,
      material,
      staged::StaticMeshAsset{
        .path = path,
        .sections = std::move(sections),
      });

    mesh_cache.insert_or_assign(
      path,
      mesh);

    return MeshRef{
      path,
      mesh};
}

std::optional<MeshRef> MeshManager::try_get(
  const assets::AssetPath& path)
{
    if(auto it = mesh_cache.find(path);
       it != mesh_cache.end())
    {
        if(auto mesh = it->second.lock())
        {
            return std::make_optional<MeshRef>(
              path,
              mesh);
        }

        // Expired entry.
        get_logger().logf(
          "Cache entry expired: '{}'",
          it->first);

        mesh_cache.erase(it);
    }

    return std::nullopt;
}

void MeshManager::process_pending()
{
    using namespace std::chrono_literals;

    auto mesh_queue = pending_upload.drain();
    for(auto& [key, entry]: mesh_queue)
    {
        if(entry->is_resolved())
        {
            // The entry was resolved externally.
            continue;
        }

        if(!entry->resources.future.valid())
        {
            // Inconsistent state:
            // Not resolved, but no future from which to obtain resources.
            get_logger().errorf(
              "Mesh '{}' has no valid loading future.",
              key);
            continue;
        }

        if(entry->resources.future.wait_for(0ms) == std::future_status::ready)
        {
            try
            {
                entry->finalize();
                get_logger().logf(
                  "Finalized mesh '{}'.",
                  key);
            }
            catch(const std::exception& error)
            {
                get_logger().errorf(
                  "Failed to finalize mesh '{}': {}",
                  key,
                  error.what());
            }

            continue;
        }

        // Mesh is still pending.
        // TODO We could place them into a temporary buffer and add them all at once.
        pending_upload.emplace_back(
          std::make_pair(
            std::move(key),
            std::move(entry)));
    }
}

void MeshManager::prune()
{
    get_logger().logf("Pruning...");

    for(auto it = mesh_cache.begin(); it != mesh_cache.end();)
    {
        if(it->second.expired())
        {
            get_logger().logf(
              "Cache entry expired: '{}'",
              it->first);

            it = mesh_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void MeshManager::clear()
{
    auto pending_meshes = pending_upload.drain();
    for(auto& [key, entry]: pending_meshes)
    {
        entry->cancel_and_wait();
    }

    pending_meshes.clear();
    mesh_cache.clear();
}

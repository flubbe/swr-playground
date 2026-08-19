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
#include "renderer/render_device.h"
#include "meshes/lod.h"
#include "colors.h"
#include "logging.h"
#include "mesh_manager.h"
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

swr::vector<StagedStaticMeshSection> build_static_mesh_sections(
  ImportedStaticMesh imported_mesh)
{
    const StaticMeshLodBuildSettings lod_settings{
      .preserve_boundaries = false,
      .recompute_normals = true,
    };

    StaticMeshLodBuilder lod_builder;
    swr::vector<StagedStaticMeshSection> sections;
    sections.reserve(imported_mesh.meshes.size());

    for(auto& mesh: imported_mesh.meshes)
    {
        const StaticMeshLodBuildResult lod_build_result =
          lod_builder.build(
            mesh.mesh_data,
            lod_settings);

        StagedStaticMeshSection section{
          .diffuse_color = mesh.diffuse_color,
          .lods = {}};
        section.lods.reserve(lod_build_result.lod_meshes.size());

        for(const auto& lod_mesh: lod_build_result.lod_meshes)
        {
            section.lods.push_back(
              StagedStaticMeshSectionLod{
                .mesh = lod_mesh.mesh,
                .bounds = calculate_mesh_bounds(lod_mesh.mesh),
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

MeshEntry::~MeshEntry()
{
    if(resolved_handles.has_value())
    {
        for(auto handle: resolved_handles.value())
        {
            device.defer_delete(handle);
        }
    }
}

void MeshEntry::finalize()
{
    if(resolved_handles.has_value())
    {
        return;
    }

    StagedStaticMeshAsset loaded = resources.future.get();
    swr::vector<StaticMeshLod> result_lods;

    bool success = false;
    auto rollback = gsl::finally(
      [&]()
      {
          if(!success)
          {
              for(auto& lod: result_lods)
              {
                  for(auto& section: lod.mesh_sections)
                  {
                      device.delete_mesh(
                        section.mesh_handle);
                  }
              }
          }
      });

    result_lods.resize(loaded.sections.front().lods.size());

    for(std::size_t i = 0; i < result_lods.size(); ++i)
    {
        result_lods[i].triangle_count =
          loaded.sections.front().lods[i].mesh.indices.size() / 3;
    }

    for(const StagedStaticMeshSection& section: loaded.sections)
    {
        for(std::size_t lod_index = 0;
            lod_index < section.lods.size()
            && lod_index < result_lods.size();
            ++lod_index)
        {
            const StagedStaticMeshSectionLod& staged_lod =
              section.lods[lod_index];
            const MeshHandle mesh_handle = device.create_mesh(staged_lod.mesh);
            expand_bounds(
              result_lods[lod_index].bounds,
              staged_lod.bounds);
            result_lods[lod_index].mesh_sections.push_back(
              MeshSection{
                .material_path = {},
                .color = section.diffuse_color,
                .mesh_handle = mesh_handle,
                .material = {material},
                .triangle_count = staged_lod.mesh.indices.size() / 3,
              });
        }
    }

    success = true;
}

void MeshEntry::release()
{
    if(!resolved_handles.has_value())
    {
        return;
    }

    for(auto& handle: resolved_handles.value())
    {
        device.delete_mesh(handle);
    }

    resolved_handles.reset();
}

/*
 * MeshManager.
 */

MeshRef MeshManager::load(
  const assets::AssetPath& path,
  MaterialRef& material)
{
    // Collect cached LOD's.
    if(auto it = mesh_cache.find(path);
       it != mesh_cache.end())
    {
        if(auto mesh = it->second.lock())
        {
            get_logger().logf(
              "Using cached mesh '{}'.",
              path);

            return MeshRef{
              path,
              mesh};
        }

        // Expired entry.
        get_logger().logf(
          "Cache entry expired: '{}'",
          it->first);

        mesh_cache.erase(it);
    }

    get_logger().logf(
      "Loading mesh '{}'.",
      path);

    // The mesh needs to be loaded. We delegate everything
    // to a task.

    auto submission = task_system.submit(
      [path = assets::AssetPath{path}](
        task_system::TaskExecutionContext& context) mutable -> StagedStaticMeshAsset
      {
          if(context.is_cancel_requested())
          {
              throw task_system::TaskCancelledError{};
          }

          auto imported_mesh = import_static_mesh(path.path);

          if(context.is_cancel_requested())
          {
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
            [](const StagedStaticMeshSection& section)
            {
                return section.lods.empty();
            });

          get_logger().logf(
            "Loaded mesh '{}'.",
            path);

          // StagedStaticMeshAsset contains only CPU-side data and can be transferred
          // to the render/main thread for finalization.

          return StagedStaticMeshAsset{
            .path = path,
            .fit_transform = ml::mat4x4::identity(),    // FIXME remove at some point?
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

std::optional<MeshRef> MeshManager::get(
  const assets::AssetPath& path)
{
    get_logger().errorf("MeshManager::get not implemented.");
    return {};
}

bool MeshManager::delete_mesh(
  const assets::AssetPath& path)
{
    get_logger().errorf("MeshManager::delete_mesh not implemented.");
    return {};
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
            entry->finalize();
            get_logger().logf(
              "Finalized mesh '{}'.",
              key);

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

/**
 * Software Rasterizer Playground.
 *
 * Mesh management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "assets/path_formatter.h"
#include "mesh_manager.h"
#include "logging.h"

namespace
{

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"MeshManager"};
    return logger;
}

}    // namespace

MeshRef MeshManager::load(
  const assets::AssetPath& path)
{
    swr::vector<swr::shared_ptr<MeshLodEntry>> lods;

    // Collect cached LOD's.
    if(auto it = mesh_cache.find(path);
       it != mesh_cache.end())
    {
        for(std::size_t i = 0; i < it->second.lods.size(); ++i)
        {
            auto& lod = it->second.lods[i];
            if(auto lod_mesh = lod.lock())
            {
                get_logger().logf(
                  "Using cached LOD {} for mesh '{}'.",
                  i,
                  path);

                lods.push_back(lod_mesh);
            }
        }

        // Expired entry.
        get_logger().logf(
          "Cache entry expired: '{}'",
          it->first);

        mesh_cache.erase(it);
    }

    get_logger().errorf("MeshManager::load not implemented.");
    return {};
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
}

void MeshManager::prune()
{
}

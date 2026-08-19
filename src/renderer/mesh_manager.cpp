/**
 * Software Rasterizer Playground.
 *
 * Mesh management.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

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

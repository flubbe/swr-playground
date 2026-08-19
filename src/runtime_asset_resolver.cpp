/**
 * Software Rasterizer Playground.
 *
 * Resolve an asset to a runtime object.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "renderer/material_manager.h"
#include "renderer/mesh_manager.h"
#include "file_manager.h"
#include "logging.h"
#include "runtime_asset_resolver.h"

// TODO Async file loading.

ResolvableMaterial RuntimeAssetResolver::resolve_material(
  const assets::AssetPath& path)
{
    return material_manager.load(
      path,
      read_text_file(file_manager, path.path));
}

MeshRef RuntimeAssetResolver::resolve_static_mesh(
  const assets::AssetPath& path)
{
    return mesh_manager.load(path);
}

TextureRef RuntimeAssetResolver::resolve_texture(
  const assets::AssetPath& path)
{
    return AssetResolver::resolve_texture(path);
}

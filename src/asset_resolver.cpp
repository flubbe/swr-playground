/**
 * Software Rasterizer Playground.
 *
 * Resolve an asset to a runtime object.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "assets/path_formatter.h"
#include "asset_resolver.h"
#include "logging.h"

ResolvableMaterial AssetResolver::resolve_material(
  const assets::AssetPath& path)
{
    logging::warningf(
      "AssetResolver::resolve_material called for '{}'",
      path);

    return ResolvableMaterial{
      path,
      nullptr};
}

MeshRef AssetResolver::resolve_static_mesh(
  const assets::AssetPath& path)
{
    logging::warningf(
      "AssetResolver::resolve_static_mesh called for '{}'",
      path);

    return {};
}

TextureRef AssetResolver::resolve_texture(
  const assets::AssetPath& path)
{
    logging::warningf(
      "AssetResolver::resolve_texture called for '{}'",
      path);

    return TextureRef{nullptr};
}

/**
 * Software Rasterizer Playground.
 *
 * Resolve an asset to a runtime object.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "assets/path.h"
#include "renderer/mesh.h"
#include "renderer/material.h"
#include "texture_cache.h"

/** Asset resolver. */
struct AssetResolver
{
    virtual ~AssetResolver() = default;

    virtual MaterialRef resolve_material(
      const assets::AssetPath& path);

    virtual MeshRef resolve_static_mesh(
      const assets::AssetPath& path,
      MaterialRef& material);

    virtual TextureRef resolve_texture(
      const assets::AssetPath& path);
};

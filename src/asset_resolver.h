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
#include "logging.h"

/*
 * Forward declarations.
 */

class ImportedStaticMesh;
class ResolvableMaterial;
class TextureRef;

// TODO Create lazily resolved mesh reference.
struct MeshRef
{
};

/** Asset resolver. */
struct AssetResolver
{
    virtual ~AssetResolver() = default;

    virtual ResolvableMaterial resolve_material(
      const assets::AssetPath& path);

    virtual MeshRef resolve_static_mesh(
      const assets::AssetPath& path);

    virtual TextureRef resolve_texture(
      const assets::AssetPath& path);
};

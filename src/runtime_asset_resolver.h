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

#include "asset_resolver.h"

/*
 * Forward declarations.
 */

class FileManager;
class MaterialManager;
class MeshManager;

/** Asset resolver. */
class RuntimeAssetResolver final
: public AssetResolver
{
    FileManager& file_manager;
    MaterialManager& material_manager;
    MeshManager& mesh_manager;

public:
    /**
     * Constructor.
     *
     * @param file_manager The file manager.
     * @param material_manager The material manager.
     * @param mesh_manager The mesh manager.
     */
    RuntimeAssetResolver(
      FileManager& file_manager,
      MaterialManager& material_manager,
      MeshManager& mesh_manager)
    : file_manager{file_manager}
    , material_manager{material_manager}
    , mesh_manager{mesh_manager}
    {
    }

    MaterialRef resolve_material(
      const assets::AssetPath& path) override;

    MeshRef resolve_static_mesh(
      const assets::AssetPath& path,
      MaterialRef& material) override;

    TextureRef resolve_texture(
      const assets::AssetPath& path) override;
};

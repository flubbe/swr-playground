/**
 * Software Rasterizer Playground.
 *
 * Staged data loaded into the runtime.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "assets/path.h"
#include "containers/vector.h"
#include "meshes/mesh.h"
#include "scene/scene.h"

#include <ml/all.h>

/*
 * Data produced by load operations before being committed to runtime state.
 */

namespace staged
{

/** LOD mesh section produced by a load operation. */
struct StaticMeshSectionLod
{
    /** Mesh data. */
    MeshData mesh;

    /** Mesh bounds. */
    MeshBounds bounds;
};

/**
 * Serialize a static mesh section level of detail.
 *
 * @param ar The archive to use.
 * @param section The mesh section lod.
 * @returns The input archive.
 */
inline serial::Archive& operator&(
  serial::Archive& ar,
  StaticMeshSectionLod& lod)
{
    ar & lod.mesh;
    ar & lod.bounds;
    return ar;
}

/** Static mesh section produced by a load operation. */
struct StaticMeshSection
{
    /** Diffuse color. */
    ml::vec4 diffuse_color{0.8f, 0.8f, 0.8f, 1.f};

    /** LODs. */
    swr::vector<StaticMeshSectionLod> lods;
};

/**
 * Serialize a static mesh section.
 *
 * @param ar The archive to use.
 * @param section The mesh section.
 * @returns The input archive.
 */
inline serial::Archive& operator&(
  serial::Archive& ar,
  StaticMeshSection& section)
{
    ar & section.diffuse_color;
    ar & section.lods;
    return ar;
}

/** Static mesh asset produced by a load operation. */
struct StaticMeshAsset
{
    /** Asset path. */
    assets::AssetPath path;

    /** Staged mesh sections. */
    swr::vector<StaticMeshSection> sections;
};

/**
 * Serialize a staged mesh asset.
 *
 * @param ar The archive to use.
 * @param mesh The mesh asset.
 * @returns The input archive.
 */
inline serial::Archive& operator&(
  serial::Archive& ar,
  StaticMeshAsset& mesh)
{
    ar & mesh.path;
    ar & mesh.sections;
    return ar;
}

/** Scene produced by a load operation, pending replacement of the active scene. */
struct StagedScene
{
    /** Loaded scene pending commit. */
    Scene scene;

    StagedScene() = default;
    StagedScene(const StagedScene&) = delete;
    StagedScene(
      StagedScene&& other)
    {
        scene.replace(std::move(other.scene));
    }

    StagedScene& operator=(const StagedScene&) = delete;
    StagedScene& operator=(StagedScene&& other)
    {
        if(this != &other)
        {
            scene.replace(std::move(other.scene));
        }

        return *this;
    }
};

}    // namespace staged

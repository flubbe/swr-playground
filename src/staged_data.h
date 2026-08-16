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

#include <mutex>
#include <optional>
#include <string>

#include "containers/string.h"
#include "containers/vector.h"
#include "meshes/mesh.h"
#include "scene/gear.h"

#include <ml/all.h>

/*
 * Generic staged data.
 */

struct StagedFloorData
{
    MeshData mesh;
};

struct StagedStaticMeshSectionLod
{
    MeshData mesh;
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
  StagedStaticMeshSectionLod& section)
{
    ar & section.mesh;
    ar & section.bounds;
    return ar;
}

struct StagedStaticMeshSection
{
    ml::vec4 diffuse_color{0.8f, 0.8f, 0.8f, 1.f};
    swr::vector<StagedStaticMeshSectionLod> lods;
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
  StagedStaticMeshSection& section)
{
    ar & section.diffuse_color;
    ar & section.lods;
    return ar;
}

struct StagedStaticMeshAsset
{
    swr::string path;
    swr::string name;
    ml::mat4x4 fit_transform{ml::mat4x4::identity()};
    swr::vector<StagedStaticMeshSection> sections;
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
  StagedStaticMeshAsset& mesh)
{
    ar & mesh.path;
    ar & mesh.name;
    ar & mesh.fit_transform;
    ar & mesh.sections;
    return ar;
}

/*
 * Concrete staged scene data (targeted to the current startup scene setup).
 */

struct StagedGearInstance
{
    ml::vec4 color;
    float inner_radius{1.f};
    float outer_radius{2.f};
    float width{1.f};
    int teeth{10};
    float tooth_depth{0.7f};
    GearGeometry geometry;
    ml::mat4x4 transform;
    ml::vec3 translation;
    float angular_speed{0.f};
    float phase_offset{0.f};
};

struct StagedStartupScene
{
    swr::vector<StagedGearInstance> gears;
    std::optional<StagedFloorData> floor;
    swr::vector<StagedStaticMeshAsset> sample_meshes;

    mutable std::mutex notices_mutex;
    swr::vector<swr::string> notices;
};

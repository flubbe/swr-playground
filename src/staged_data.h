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
#include "assets/texture.h"
#include "mesh.h"
#include "ml/all.h"
#include "scene/gear.h"

/*
 * Generic staged data.
 */

struct StagedFloorData
{
    MeshData mesh;
    assets::ImageRGBA8 diffuse_texture;
    assets::ImageRGBA8 normal_texture;
};

struct StagedStaticMeshSectionLod
{
    MeshData mesh;
    float min_screen_height{0.f};
    MeshBounds bounds;
};

struct StagedStaticMeshSection
{
    ml::vec4 diffuse_color{0.8f, 0.8f, 0.8f, 1.f};
    swr::vector<StagedStaticMeshSectionLod> lods;
};

struct StagedStaticMeshAsset
{
    swr::string name;
    ml::mat4x4 fit_transform{ml::mat4x4::identity()};
    swr::vector<StagedStaticMeshSection> sections;
};

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
    std::optional<StagedStaticMeshAsset> sample_mesh;

    mutable std::mutex notices_mutex;
    swr::vector<swr::string> notices;
};

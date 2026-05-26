/**
 * Software Rasterizer Playground.
 *
 * Static mesh import data.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "mesh.h"
#include "ml/all.h"

struct ImportedMesh
{
    std::string name;
    MeshData mesh_data;
    ml::vec4 diffuse_color{0.8f, 0.8f, 0.8f, 1.0f};
};

struct ImportedStaticMesh
{
    std::vector<ImportedMesh> meshes;
};

ImportedStaticMesh import_static_mesh(
  const std::filesystem::path& path);

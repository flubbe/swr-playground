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

#include "ml/all.h"

#include "containers/string.h"
#include "containers/vector.h"
#include "mesh.h"

struct ImportedMesh
{
    swr::string name;
    MeshData mesh_data;
    ml::vec4 diffuse_color{0.8f, 0.8f, 0.8f, 1.0f};
};

struct ImportedStaticMesh
{
    swr::vector<ImportedMesh> meshes;
};

ImportedStaticMesh import_static_mesh(
  const std::filesystem::path& path);

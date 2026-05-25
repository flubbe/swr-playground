/**
 * Software Rasterizer Playground.
 *
 * Static mesh importer implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "static_mesh_importer.h"

namespace
{

ml::vec4 to_vec4_position(const aiVector3D& v)
{
    return {v.x, v.y, v.z, 1.0f};
}

ml::vec4 to_vec4_normal(const aiVector3D& v)
{
    return {v.x, v.y, v.z, 0.0f};
}

ml::vec4 read_diffuse_color(
  const aiScene& scene,
  unsigned int material_index)
{
    if(material_index >= scene.mNumMaterials
       || scene.mMaterials[material_index] == nullptr)
    {
        return {0.8f, 0.8f, 0.8f, 1.0f};
    }

    aiColor4D color{};
    if(AI_SUCCESS
       == aiGetMaterialColor(
         scene.mMaterials[material_index],
         AI_MATKEY_COLOR_DIFFUSE,
         &color))
    {
        return {color.r, color.g, color.b, color.a};
    }

    return {0.8f, 0.8f, 0.8f, 1.0f};
}

MeshData import_mesh_data(const aiMesh& mesh)
{
    MeshData result{
      .primitive_type = PrimitiveType::Triangles,
    };

    result.vertices.reserve(mesh.mNumVertices);
    result.normals.reserve(mesh.mNumVertices);
    for(unsigned int i = 0; i < mesh.mNumVertices; ++i)
    {
        result.vertices.emplace_back(to_vec4_position(mesh.mVertices[i]));
        result.normals.emplace_back(
          mesh.HasNormals()
            ? to_vec4_normal(mesh.mNormals[i])
            : ml::vec4{0.0f, 0.0f, 1.0f, 0.0f});
    }

    result.indices.reserve(static_cast<std::size_t>(mesh.mNumFaces) * 3u);
    for(unsigned int face_index = 0; face_index < mesh.mNumFaces; ++face_index)
    {
        const aiFace& face = mesh.mFaces[face_index];
        if(face.mNumIndices != 3)
        {
            continue;
        }

        result.indices.emplace_back(face.mIndices[0]);
        result.indices.emplace_back(face.mIndices[1]);
        result.indices.emplace_back(face.mIndices[2]);
    }

    return result;
}

}    // namespace

ImportedStaticMesh import_static_mesh(
  const std::filesystem::path& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
      path.string(),
      aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenSmoothNormals
        | aiProcess_SortByPType
        | aiProcess_ValidateDataStructure);

    if(scene == nullptr)
    {
        throw std::runtime_error{
          std::format(
            "Unable to import static mesh: {}",
            importer.GetErrorString())};
    }

    ImportedStaticMesh result;
    result.meshes.reserve(scene->mNumMeshes);
    for(unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
    {
        const aiMesh* mesh = scene->mMeshes[mesh_index];
        if(mesh == nullptr || mesh->mNumVertices == 0)
        {
            continue;
        }

        MeshData mesh_data = import_mesh_data(*mesh);
        if(mesh_data.indices.empty())
        {
            continue;
        }

        result.meshes.push_back(
          ImportedMesh{
            .name = mesh->mName.C_Str(),
            .mesh_data = std::move(mesh_data),
            .diffuse_color = read_diffuse_color(*scene, mesh->mMaterialIndex),
          });
    }

    return result;
}

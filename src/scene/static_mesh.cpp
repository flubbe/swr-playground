/**
 * Software Rasterizer Playground.
 *
 * Static mesh object implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <utility>

#include "assets/path_formatter.h"
#include "reflection/builtin_properties.h"
#include "scene/properties.h"
#include "asset_resolver.h"
#include "logging.h"
#include "static_mesh.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(StaticMesh);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void StaticMesh::register_properties(
  reflect::ClassInfo& class_info)
{
    reflect::register_property<&StaticMesh::path>(
      class_info,
      "path",
      "Asset Path",
      reflect::PropertyFlags::ReadOnly);
    reflect::register_property<&StaticMesh::materials>(
      class_info,
      "materials",
      "Material Paths");
    reflect::register_property<&StaticMesh::casts_shadows>(
      class_info,
      "casts_shadows",
      "Casts Shadows");
    reflect::register_property<&StaticMesh::receives_shadows>(
      class_info,
      "receives_shadows",
      "Receives Shadows");
}

void StaticMesh::resolve(
  AssetResolver& resolver)
{
    mesh_lods.clear();

    if(path.path.empty())
    {
        // Parametric asset.
        return;
    }

    // TODO

    if(materials.empty())
    {
        logging::warningf(
          "StaticMesh '{}' doesn't declare materials.",
          path);

        return;
    }

    swr::vector<MaterialRef> material_refs;
    for(auto& material_path: materials)
    {
        material_refs.emplace_back(
          resolver.resolve_material(
            material_path));
    }

    // TODO pick first material.
    resolver.resolve_static_mesh(
      path,
      material_refs[0]);
}

void StaticMesh::post_load()
{
    // TODO Construct mesh/LODs from path.
    //      Likely should be conditional for meshes defined inline?

    logging::warningf("StaticMesh::post_load");
}

void StaticMesh::init(
  const assets::AssetPath& path,
  const swr::vector<assets::AssetPath>& materials,
  swr::vector<MeshSection> sections,
  MeshBounds bounds)
{
    this->path = path;
    this->materials = materials;
    set_lods(
      {StaticMeshLod{
        .mesh_sections = std::move(sections),
        .bounds = bounds}});
}

void StaticMesh::init(
  const assets::AssetPath& path,
  const swr::vector<assets::AssetPath>& materials,
  swr::vector<StaticMeshLod> lods)
{
    this->path = path;
    this->materials = materials;
    set_lods(std::move(lods));
}

void StaticMesh::set_lods(
  swr::vector<StaticMeshLod> lods)
{
    mesh_lods = std::move(lods);
    update_bounds();
}

void StaticMesh::clear_mesh_sections() noexcept
{
    mesh_lods.clear();
    update_bounds();
}

void StaticMesh::update_bounds() noexcept
{
    mesh_bounds = {};
    for(const StaticMeshLod& lod: mesh_lods)
    {
        if(!lod.mesh_sections.empty()
           && lod.bounds.valid)
        {
            mesh_bounds = lod.bounds;
            return;
        }
    }
}

std::size_t StaticMesh::select_lod(
  float projected_pixel_area,
  float target_pixels_per_triangle) const noexcept
{
    if(mesh_lods.empty())
    {
        return 0;
    }

    std::size_t fallback = 0;
    if(target_pixels_per_triangle <= 0)
    {
        return fallback;
    }

    projected_pixel_area = std::max(0.0f, projected_pixel_area);

    bool found_renderable = false;

    for(std::size_t lod_index = 0; lod_index < mesh_lods.size(); ++lod_index)
    {
        const StaticMeshLod& lod = mesh_lods[lod_index];
        if(lod.mesh_sections.empty() || lod.triangle_count == 0)
        {
            continue;
        }

        fallback = lod_index;
        found_renderable = true;

        const float pixels_per_triangle =
          projected_pixel_area / static_cast<float>(lod.triangle_count);

        if(pixels_per_triangle >= target_pixels_per_triangle)
        {
            return lod_index;
        }
    }

    return found_renderable ? fallback : 0;
}

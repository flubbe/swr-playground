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
#include "renderer/mesh_manager.h"
#include "scene/properties.h"
#include "asset_resolver.h"
#include "logging.h"
#include "scene.h"
#include "static_mesh.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(StaticMesh);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void StaticMesh::register_properties(
  reflect::ClassInfo& class_info)
{
    class_info.register_property<&StaticMesh::path>(
      "path",
      "Asset Path",
      reflect::PropertyFlags::ReadOnly);
    class_info.register_property<&StaticMesh::materials>(
      "materials",
      "Material Paths");
    class_info.register_property<&StaticMesh::casts_shadows>(
      "casts_shadows",
      "Casts Shadows");
    class_info.register_property<&StaticMesh::receives_shadows>(
      "receives_shadows",
      "Receives Shadows");
}

void StaticMesh::resolve(
  AssetResolver& resolver)
{
    mesh_lods.clear();
    material_ref.reset();

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

    material_ref = material_refs[0];

    if(path.path.empty())
    {
        // Parametric asset.
        return;
    }

    // TODO pick first material.
    mesh_ref = resolver.resolve_static_mesh(
      path,
      material_refs[0]);
}

void StaticMesh::post_load()
{
    mark_mesh_dirty();
}

void StaticMesh::release()
{
    Super::release();

    materials.clear();
    material_ref.reset();
    mesh_lods.clear();
    mesh_ref.reset();
    pending_mesh_ref.reset();
}

void StaticMesh::on_properties_changed()
{
    pending_mesh_ref.reset();
    mark_mesh_dirty();
}

void StaticMesh::init(
  const assets::AssetPath& path,
  const swr::vector<assets::AssetPath>& materials,
  MeshRef mesh)
{
    this->path = path;
    this->materials = materials;
    mesh_ref = mesh;
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
    for(const auto& lod: lods)
    {
        for(const auto& section: lod.mesh_sections)
        {
            if(section.material)
            {
                material_ref = section.material;
                break;
            }
        }

        if(material_ref.has_value())
        {
            break;
        }
    }

    mesh_lods = std::move(lods);
}

void StaticMesh::mark_mesh_dirty()
{
    mesh_dirty = true;

    if(scene != nullptr)
    {
        scene->mark_mesh_dirty(object_id);
    }
}

void StaticMesh::clear_mesh_dirty()
{
    mesh_dirty = false;
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

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
    sections.clear();
    bounds.valid = false;

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
    sections.clear();
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
  swr::vector<MeshSection> sections)
{
    this->path = path;
    this->materials = materials;
    set_sections(std::move(sections));
}

void StaticMesh::set_sections(
  swr::vector<MeshSection> sections)
{
    bounds = {};

    for(const auto& section: sections)
    {
        expand_bounds(
          bounds,
          section.bounds);

        if(section.material)
        {
            material_ref = section.material;
        }
    }

    this->sections = std::move(sections);
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

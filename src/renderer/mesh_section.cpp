/**
 * Software Rasterizer Playground.
 *
 * A mesh section, that is, a mesh handle with materials and potentially other metadata.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <format>
#include <stdexcept>

#include "asset_resolver.h"
#include "material_manager.h"
#include "mesh_section.h"

#include "logging.h"

void MeshSection::resolve(
  AssetResolver& resolver)
{
    // For materials not externally loaded.
    if(material_path.path.empty())
    {
        return;
    }

    // TODO

    logging::warningf("MeshSection::resolve");
}

void MeshSection::post_load(
  MaterialManager& material_manager)
{
    // For materials not externally loaded.
    if(material_path.path.empty())
    {
        return;
    }

    auto result = material_manager.get(
      material_path.path.string());

    if(!result.has_value())
    {
        throw std::runtime_error{
          std::format(
            "Could not resolve material '{}' for mesh section.",
            material_path.path.string())};
    }
}

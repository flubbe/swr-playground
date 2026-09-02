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

#include "assets/path_formatter.h"
#include "asset_resolver.h"
#include "material_manager.h"
#include "mesh_section.h"

#include "logging.h"

void MeshSection::resolve(
  AssetResolver& resolver)
{
    // TODO

    logging::warningf("MeshSection::resolve");
}

void MeshSection::post_load(
  MaterialManager& material_manager)
{
}

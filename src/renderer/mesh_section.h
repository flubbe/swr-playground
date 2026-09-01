/**
 * Software Rasterizer Playground.
 *
 * A mesh section, that is, a mesh handle with materials and potentially other metadata.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <ml/all.h>

#include "assets/path.h"
#include "material.h"
#include "types.h"

/*
 * Forward declarations.
 */

struct AssetResolver;

/** Part of a mesh using one material. */
struct MeshSection
{
    /*
     * Serialized.
     */

    /** Material path. */
    assets::AssetPath material_path;

    /** Base color used by the lighting shader. */
    ml::vec4 color{1.f, 1.f, 1.f, 1.f};

    /*
     * Runtime.
     */

    /** Mesh handle. */
    MeshHandle mesh_handle;

    /** Material reference. */
    MaterialRef material;

    /*
     * Metadata.
     */

    /** Triangle count in this level of detail. */
    std::size_t triangle_count{0};

    /** Dependency resolution. */
    void resolve(AssetResolver& resolver);

    /** Process section after loading. */
    void post_load(
      class MaterialManager& material_manager);
};

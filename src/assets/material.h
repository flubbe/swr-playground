/**
 * Software Rasterizer Playground.
 *
 * Material asset loading helpers.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <filesystem>
#include <optional>

#include "containers/string.h"
#include "containers/vector.h"
#include "path.h"
#include "texture.h"

namespace assets
{

/** Description of a tangent-space normal map. */
struct NormalMapDesc
{
    /** Path. */
    AssetPath path;

    /** Normal map convention. Either `OpenGL` or `DirectX`. */
    NormalMapConvention convention{NormalMapConvention::OpenGL};

    /** Scale. */
    float scale{1.f};
};

/** Material description. */
struct MaterialDesc
{
    /** Optional material display name. */
    std::optional<swr::string> name;

    /** Shader identifier. */
    swr::string shader;

    /** Optional sRGB base-color texture. */
    std::optional<AssetPath> base_color;

    /** Optional tangent-space normal map. */
    std::optional<NormalMapDesc> normal;
};

/**
 * Load a material from a JSON string.
 *
 * @param json The JSON string.
 * @returns Returns the material.
 */
[[nodiscard]]
MaterialDesc load_material(
  std::string_view json);

}    // namespace assets

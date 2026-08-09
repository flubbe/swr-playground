/**
 * Software Rasterizer Playground.
 *
 * Material asset loading helpers.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <filesystem>
#include <span>

#include "containers/string.h"
#include "containers/vector.h"

namespace assets
{

/** Material description. */
struct MaterialDesc
{
    /** Shader identifier. */
    swr::string shader;

    /** Textures. */
    swr::vector<std::filesystem::path> textures;
};

/**
 * Load a material from a JSON string.
 *
 * @param json The JSON string.
 * @returns Returns the material.
 */
[[nodiscard]]
MaterialDesc load_material(
  const swr::string& json);

}    // namespace assets

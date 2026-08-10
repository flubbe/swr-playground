/**
 * Software Rasterizer Playground.
 *
 * Material definition.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "containers/vector.h"
#include "types.h"

/** A render material. */
struct Material
{
    /** Shader handle. */
    ShaderHandle shader_handle{0};

    /** Bound 2D textures by texture unit index. */
    swr::vector<TextureHandle> texture_handles{};
};

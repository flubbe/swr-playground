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

#include "types.h"

/** A render device material state used to create a render material. */
struct Material
{
    /** Shader handle. */
    ShaderHandle shader_handle{0};

    /** Texture bound to sampler unit 0. */
    TextureHandle base_color_handle{};

    /** Texture bound to sampler unit 1. */
    TextureHandle normal_map_handle{};
};

/**
 * Software Rasterizer Playground.
 *
 * Render-facing shared types.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstdint>

#include "ml/all.h"

/** Part of a mesh using one material. */
struct MeshSection
{
    /** Mesh handle. */
    std::uint32_t mesh_handle{0};

    /** Material handle. */
    std::uint32_t material_handle{0};

    /** Base color used by the lighting shader. */
    ml::vec4 color{1.f, 1.f, 1.f, 1.f};
};

/** Handle to a shadow-map render target owned by the render device. */
using ShadowMapHandle = std::uint32_t;

/** One shadow map input for a draw call. */
struct ShadowMapBinding
{
    bool enabled{false};
    ShadowMapHandle handle{0};
    ml::mat4x4 clip_from_mesh{ml::mat4x4::identity()};
    float depth_bias{0.0015f};
    bool linear_filter{false};
};

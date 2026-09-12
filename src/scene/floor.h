/**
 * Software Rasterizer Playground.
 *
 * Create procedural floor geometry.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "static_mesh.h"

/** A procedural floor object. */
class Floor
: public reflect::Reflected<Floor, StaticMesh>
{
    float half_extent{28.f};
    float uv_repeat{1.f};

public:
    /** Property registration hook. */
    static void register_properties(reflect::ClassInfo& class_info);

    Floor() = default;

    void post_load() override;

    [[nodiscard]]
    MeshData generate_mesh() const;

    [[nodiscard]]
    float get_half_extent() const noexcept
    {
        return half_extent;
    }

    [[nodiscard]]
    float get_uv_repeat() const noexcept
    {
        return uv_repeat;
    }
};

DECLARE_REFLECTION(Scene, Floor);

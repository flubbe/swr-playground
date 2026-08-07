/**
 * Software Rasterizer Playground.
 *
 * Shared light functionality.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <ml/all.h>

#include "object.h"

class Light
: public reflect::Reflected<Light, Object>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    Light() = default;

    [[nodiscard]]
    ml::vec3 get_world_forward_direction() const
    {
        const ml::vec3 direction =
          (get_transform() * ml::vec4{0.f, 0.f, -1.f, 0.f}).xyz();
        if(direction.length() <= 0.0001f)
        {
            return {0.f, 0.f, -1.f};
        }
        return direction.normalized();
    }
};

DECLARE_REFLECTION(Scene, Light);

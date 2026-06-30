/**
 * Software Rasterizer Playground.
 *
 * Directional light object.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "light.h"

enum class DirectionalLightBehavior
{
    Rotating,
    Stationary,
};

class DirectionalLight
: public reflect::Reflected<DirectionalLight, Light>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    bool enabled{true};
    DirectionalLightBehavior behavior{DirectionalLightBehavior::Rotating};
    float brightness{0.5f};

    [[nodiscard]]
    ml::vec3 get_world_direction_to_light() const
    {
        return -get_world_forward_direction();
    }
};

DECLARE_REFLECTION(Scene, DirectionalLight);

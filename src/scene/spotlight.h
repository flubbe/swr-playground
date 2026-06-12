/**
 * Software Rasterizer Playground.
 *
 * Spot light object.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "light.h"

class SpotLight
: public reflect::Reflected<SpotLight, Light>
{
    float cone_angle_radians{ml::to_radians(35.f)};
    float range{30.f};

public:
    static void register_properties(reflect::ClassInfo& class_info);

    SpotLight() = default;
};

DECLARE_REFLECTION(Scene, SpotLight);

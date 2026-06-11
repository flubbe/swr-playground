/**
 * Software Rasterizer Playground.
 *
 * A simple light.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "ml/all.h"
#include "object.h"

enum class LightType
{
    Rotating,
    Stationary,
};

class Light
: public reflect::Reflected<Light, Object>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    /** Light behavior mode. */
    LightType type{LightType::Rotating};

    /** light position. */
    ml::vec4 position{5.0f, 5.0f, 10.0f, 1.0f};

    Light() = default;
};

DECLARE_REFLECTION(Scene, Light);

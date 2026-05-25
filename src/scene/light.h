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

class Light
: public reflect::Reflected<Light, Object>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    /** light position. */
    ml::vec4 position{5.0f, 5.0f, 10.0f, 0.0f};

    Light() = default;
};

DECLARE_REFLECTION(Scene, Light);

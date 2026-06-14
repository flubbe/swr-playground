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
public:
    static void register_properties(reflect::ClassInfo& class_info);

    bool enabled{true};
    ml::vec4 color{0.42f, 0.62f, 1.f, 1.f};
    float brightness{1.4f};
    float inner_cone_angle_radians{ml::to_radians(20.f)};
    float outer_cone_angle_radians{ml::to_radians(35.f)};
    float range{30.f};

    SpotLight() = default;

    [[nodiscard]]
    float get_inner_cone_angle_radians() const noexcept
    {
        return std::min(inner_cone_angle_radians, outer_cone_angle_radians);
    }

    [[nodiscard]]
    float get_outer_cone_angle_radians() const noexcept
    {
        return std::max(inner_cone_angle_radians, outer_cone_angle_radians);
    }

    [[nodiscard]]
    float get_range() const noexcept
    {
        return range;
    }

    [[nodiscard]]
    const ml::vec4& get_color() const noexcept
    {
        return color;
    }

    [[nodiscard]]
    ml::vec3 get_world_spot_direction() const
    {
        return get_world_forward_direction();
    }
};

DECLARE_REFLECTION(Scene, SpotLight);

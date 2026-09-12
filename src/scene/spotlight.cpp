/**
 * Software Rasterizer Playground.
 *
 * Spot light object implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "spotlight.h"

#include "reflection/builtin_properties.h"
#include "scene/properties.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(SpotLight);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void SpotLight::register_properties(reflect::ClassInfo& class_info)
{
    class_info.register_property<&SpotLight::enabled>(
      "enabled",
      "Enabled");
    class_info.register_property<&SpotLight::casts_shadows>(
      "casts_shadows",
      "Casts Shadows");
    class_info.register_property<&SpotLight::color>(
      "color",
      "Color");

    reflect::RangeConstraint<float> brightness_constraints{};
    brightness_constraints.min = 0.f;
    brightness_constraints.max = 32.f;
    brightness_constraints.clamp = true;

    class_info.register_property<&SpotLight::brightness>(
      "brightness",
      "Brightness",
      reflect::PropertyFlags::None,
      brightness_constraints);

    reflect::RangeConstraint<float> inner_angle_constraints{};
    inner_angle_constraints.min = ml::to_radians(1.f);
    inner_angle_constraints.max = ml::to_radians(89.f);
    inner_angle_constraints.clamp = true;

    reflect::RangeConstraint<float> outer_angle_constraints{};
    outer_angle_constraints.min = ml::to_radians(1.f);
    outer_angle_constraints.max = ml::to_radians(89.f);
    outer_angle_constraints.clamp = true;

    reflect::RangeConstraint<float> range_constraints{};
    range_constraints.min = 0.1f;
    range_constraints.clamp = true;

    class_info.register_property<&SpotLight::inner_cone_angle_radians>(
      "inner_cone_angle_radians",
      "Inner Cone Angle (rad)",
      reflect::PropertyFlags::None,
      inner_angle_constraints);
    class_info.register_property<&SpotLight::outer_cone_angle_radians>(
      "outer_cone_angle_radians",
      "Outer Cone Angle (rad)",
      reflect::PropertyFlags::None,
      outer_angle_constraints);
    class_info.register_property<&SpotLight::range>(
      "range",
      "Range",
      reflect::PropertyFlags::None,
      range_constraints);
}

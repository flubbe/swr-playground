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
    reflect::register_property<&SpotLight::enabled>(
      class_info,
      "enabled",
      "Enabled");
    reflect::register_property<&SpotLight::color>(
      class_info,
      "color",
      "Color");

    reflect::RangeConstraint<float> brightness_constraints{};
    brightness_constraints.min = 0.f;
    brightness_constraints.max = 32.f;
    brightness_constraints.clamp = true;

    reflect::register_property<&SpotLight::brightness>(
      class_info,
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

    reflect::register_property<&SpotLight::inner_cone_angle_radians>(
      class_info,
      "inner_cone_angle_radians",
      "Inner Cone Angle (rad)",
      reflect::PropertyFlags::None,
      inner_angle_constraints);
    reflect::register_property<&SpotLight::outer_cone_angle_radians>(
      class_info,
      "outer_cone_angle_radians",
      "Outer Cone Angle (rad)",
      reflect::PropertyFlags::None,
      outer_angle_constraints);
    reflect::register_property<&SpotLight::range>(
      class_info,
      "range",
      "Range",
      reflect::PropertyFlags::None,
      range_constraints);
}

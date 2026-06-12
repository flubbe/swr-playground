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
    reflect::RangeConstraint<float> angle_constraints{};
    angle_constraints.min = ml::to_radians(1.f);
    angle_constraints.max = ml::to_radians(89.f);
    angle_constraints.clamp = true;

    reflect::RangeConstraint<float> range_constraints{};
    range_constraints.min = 0.1f;
    range_constraints.clamp = true;

    reflect::register_property<&SpotLight::cone_angle_radians>(
      class_info,
      "cone_angle_radians",
      "Cone Angle (rad)",
      reflect::PropertyFlags::None,
      angle_constraints);
    reflect::register_property<&SpotLight::range>(
      class_info,
      "range",
      "Range",
      reflect::PropertyFlags::None,
      range_constraints);
}

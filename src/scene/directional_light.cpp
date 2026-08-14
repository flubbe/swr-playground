/**
 * Software Rasterizer Playground.
 *
 * Directional light object implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "directional_light.h"

#include "reflection/builtin_properties.h"
#include "scene/properties.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(DirectionalLight);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void DirectionalLight::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&DirectionalLight::enabled>(
      class_info,
      "enabled",
      "Enabled");

    reflect::RangeConstraint<float> brightness_constraints{};
    brightness_constraints.min = 0.f;
    brightness_constraints.max = 4.f;
    brightness_constraints.clamp = true;

    reflect::register_property<&DirectionalLight::brightness>(
      class_info,
      "brightness",
      "Brightness",
      reflect::PropertyFlags::None,
      brightness_constraints);
}

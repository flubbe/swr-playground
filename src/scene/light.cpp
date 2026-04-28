/**
 * Software Rasterizer Playground.
 *
 * Light implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "light.h"

#include "scene/properties.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Light);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void Light::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&Light::position>(
      class_info,
      "position",
      "Position");
}

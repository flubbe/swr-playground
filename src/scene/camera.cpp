/**
 * Software Rasterizer Playground.
 *
 * Camera model.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "reflection/builtin_properties.h"
#include "camera.h"
#include "properties.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Camera)

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void Camera::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&Camera::width>(
      class_info,
      "width",
      "Width");
    reflect::register_property<&Camera::height>(
      class_info,
      "height",
      "Height");
    reflect::register_property<&Camera::proj>(
      class_info,
      "proj",
      "Projection Matrix");
}

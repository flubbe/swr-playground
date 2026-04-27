/**
 * Software Rasterizer Playground.
 *
 * Object implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <ranges>

#include "reflection/builtin_properties.h"
#include "scene/properties.h"
#include "object.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Object);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void Object::register_properties(reflect::ClassInfo& class_info)
{
    register_property<&Object::object_id>(
      class_info,
      "object_id",
      "Object ID",
      reflect::PropertyFlags::ReadOnly);
    register_property<&Object::name>(
      class_info,
      "name",
      "Name");
    register_property<&Object::transform>(
      class_info,
      "transform",
      "Transform");
}

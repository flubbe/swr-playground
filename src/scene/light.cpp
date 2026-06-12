/**
 * Software Rasterizer Playground.
 *
 * Shared light functionality.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "light.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Light);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void Light::register_properties(
  [[maybe_unused]] reflect::ClassInfo& class_info)
{
}

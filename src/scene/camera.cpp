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
    reflect::register_property<&Camera::fov_y>(
      class_info,
      "fov_y",
      "FOV Y (rad)");
    reflect::register_property<&Camera::near_plane>(
      class_info,
      "near_plane",
      "Near Plane");
    reflect::register_property<&Camera::far_plane>(
      class_info,
      "far_plane",
      "Far Plane");
}

ml::mat4x4 Camera::get_projection_matrix(float aspect_ratio) const
{
    if(aspect_ratio <= 0.f)
    {
        aspect_ratio = 1.f;
    }

    return ml::matrices::perspective_projection(
      aspect_ratio,
      fov_y,
      near_plane,
      far_plane);
}

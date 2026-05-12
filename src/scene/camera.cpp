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
    reflect::RangeConstraint<float> fov_constraints{};
    fov_constraints.min = 0.1;
    fov_constraints.max = M_PI;
    fov_constraints.clamp = true;

    reflect::RangeConstraint<float> plane_constraints{};
    plane_constraints.min = 0.1;
    plane_constraints.clamp = true;

    reflect::register_property<&Camera::fov_y>(
      class_info,
      "fov_y",
      "FOV Y (rad)",
      reflect::PropertyFlags::None,
      fov_constraints);
    reflect::register_property<&Camera::near_plane>(
      class_info,
      "near_plane",
      "Near Plane",
      reflect::PropertyFlags::None,
      plane_constraints);
    reflect::register_property<&Camera::far_plane>(
      class_info,
      "far_plane",
      "Far Plane",
      reflect::PropertyFlags::None,
      plane_constraints);
}

Camera::Camera()
: reflect::Reflected<Camera, Object>{Camera::static_class()}
{
    update_projection_matrix(1.f);
}

void Camera::on_properties_changed()
{
    update_projection_matrix(cached_aspect_ratio);
}

void Camera::update_projection_matrix(float aspect_ratio)
{
    if(aspect_ratio <= 0.f)
    {
        aspect_ratio = 1.f;
    }

    if(cached_aspect_ratio == aspect_ratio
       && cached_fov_y == fov_y
       && cached_near_plane == near_plane
       && cached_far_plane == far_plane)
    {
        return;
    }

    cached_projection = ml::matrices::perspective_projection(
      aspect_ratio,
      fov_y,
      near_plane,
      far_plane);
    cached_aspect_ratio = aspect_ratio;
    cached_fov_y = fov_y;
    cached_near_plane = near_plane;
    cached_far_plane = far_plane;
}

ml::mat4x4 Camera::get_projection_matrix() const
{
    return cached_projection;
}

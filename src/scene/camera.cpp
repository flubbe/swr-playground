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

#include <algorithm>

#include "camera.h"
#include "logging.h"
#include "properties.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Camera)

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void Camera::post_load()
{
    Super::post_load();

    // TODO constuct e.g. perspective matrix
    logging::warningf("Camera::post_load");
}

void Camera::register_properties(reflect::ClassInfo& class_info)
{
    reflect::RangeConstraint<float> fov_constraints{};
    fov_constraints.min = 0.1;
    fov_constraints.max = std::numbers::pi_v<float>;
    fov_constraints.clamp = true;

    reflect::RangeConstraint<float> orthographic_height_constraints{};
    orthographic_height_constraints.min = 0.1;
    orthographic_height_constraints.clamp = true;

    reflect::RangeConstraint<float> plane_constraints{};
    plane_constraints.min = 0.1;
    plane_constraints.clamp = true;

    reflect::register_property<&Camera::fov_y>(
      class_info,
      "fov_y",
      "FOV Y (rad)",
      reflect::PropertyFlags::None,
      fov_constraints);
    reflect::register_property<&Camera::orthographic_height>(
      class_info,
      "orthographic_height",
      "Orthographic Height",
      reflect::PropertyFlags::None,
      orthographic_height_constraints);
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
{
    update_projection_matrix(1.f);
}

void Camera::on_properties_changed()
{
    Super::on_properties_changed();

    update_projection_matrix(cached_aspect_ratio);
}

void Camera::update_projection_matrix(float aspect_ratio)
{
    if(aspect_ratio <= 0.f)
    {
        aspect_ratio = 1.f;
    }

    if(cached_aspect_ratio == aspect_ratio
       && cached_projection_mode == projection_mode
       && cached_fov_y == fov_y
       && cached_orthographic_height == orthographic_height
       && cached_near_plane == near_plane
       && cached_far_plane == far_plane)
    {
        return;
    }

    if(projection_mode == CameraProjectionMode::Orthographic)
    {
        const float half_height = orthographic_height * 0.5f;
        const float half_width = half_height * aspect_ratio;
        cached_projection = ml::matrices::orthographic_projection(
          -half_width,
          half_width,
          -half_height,
          half_height,
          near_plane,
          far_plane);
    }
    else
    {
        cached_projection = ml::matrices::perspective_projection(
          aspect_ratio,
          fov_y,
          near_plane,
          far_plane);
    }

    cached_aspect_ratio = aspect_ratio;
    cached_projection_mode = projection_mode;
    cached_fov_y = fov_y;
    cached_orthographic_height = orthographic_height;
    cached_near_plane = near_plane;
    cached_far_plane = far_plane;
}

ml::mat4x4 Camera::get_projection_matrix() const
{
    return cached_projection;
}

void Camera::set_projection_mode(CameraProjectionMode mode)
{
    projection_mode = mode;
}

CameraProjectionMode Camera::get_projection_mode() const noexcept
{
    return projection_mode;
}

void Camera::set_orthographic_height(float height)
{
    orthographic_height = std::max(0.1f, height);
}

float Camera::get_orthographic_height() const noexcept
{
    return orthographic_height;
}

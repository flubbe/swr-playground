/**
 * Software Rasterizer Playground.
 *
 * Viewport.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "viewport.h"

#include <algorithm>
#include <cmath>

#include "scene/scene.h"

namespace
{

constexpr float editor_camera_default_orbit_distance = 40.f;
constexpr float editor_camera_default_orthographic_height = 40.f;

ProjectionType get_projection_type_for_view(EditorCameraView view)
{
    if(view == EditorCameraView::Perspective)
    {
        return ProjectionType::Perspective;
    }

    return ProjectionType::Orthographic;
}

ml::vec3 make_forward_direction(
  float pitch_radians,
  float yaw_radians)
{
    const float cos_pitch = std::cos(pitch_radians);
    const float sin_pitch = std::sin(pitch_radians);
    const float cos_yaw = std::cos(yaw_radians);
    const float sin_yaw = std::sin(yaw_radians);

    return {
      sin_yaw * cos_pitch,
      -sin_pitch,
      -cos_yaw * cos_pitch};
}

ml::vec3 make_right_direction(float yaw_radians)
{
    return {
      std::cos(yaw_radians),
      0.f,
      std::sin(yaw_radians)};
}

ml::vec3 make_up_direction(
  float pitch_radians,
  float yaw_radians)
{
    const ml::vec3 forward = make_forward_direction(pitch_radians, yaw_radians);
    const ml::vec3 right = make_right_direction(yaw_radians);
    return right.cross_product(forward).normalized();
}

void align_fps_position_to_orbit_view(
  Viewport::EditorCameraControllerState& controller)
{
    const ml::vec3 forward = make_forward_direction(
      controller.pitch_radians,
      controller.yaw_radians);
    controller.position =
      controller.orbit_target - forward * controller.orbit_distance;
}

void align_orbit_target_to_fps_view(
  Viewport::EditorCameraControllerState& controller)
{
    const ml::vec3 forward = make_forward_direction(
      controller.pitch_radians,
      controller.yaw_radians);
    controller.orbit_target =
      controller.position + forward * controller.orbit_distance;
}

void configure_controller_for_view(
  Viewport::EditorCameraControllerState& controller,
  EditorCameraView view)
{
    controller.orbit_target = {0.f, 0.f, 0.f};
    controller.orbit_distance = editor_camera_default_orbit_distance;
    controller.orthographic_height = editor_camera_default_orthographic_height;

    switch(view)
    {
    case EditorCameraView::Perspective:
    case EditorCameraView::Orthographic:
        controller.pitch_radians = ml::to_radians(20.f);
        controller.yaw_radians = ml::to_radians(30.f);
        break;
    case EditorCameraView::Top:
        controller.pitch_radians = ml::to_radians(90.f);
        controller.yaw_radians = 0.f;
        break;
    case EditorCameraView::Left:
        controller.pitch_radians = 0.f;
        controller.yaw_radians = ml::to_radians(90.f);
        break;
    case EditorCameraView::Front:
        controller.pitch_radians = 0.f;
        controller.yaw_radians = 0.f;
        break;
    }

    align_fps_position_to_orbit_view(controller);
}

void reset_editor_camera_controller(
  Viewport::EditorCameraControllerState& controller,
  EditorCameraView view)
{
    controller = {};
    configure_controller_for_view(controller, view);
}

void synchronize_controller_to_navigation_mode(
  Viewport::EditorCameraControllerState& controller,
  ViewportNavigationMode mode)
{
    if(mode == ViewportNavigationMode::Orbit)
    {
        align_orbit_target_to_fps_view(controller);
        return;
    }

    align_fps_position_to_orbit_view(controller);
}

ml::mat4x4 make_view_matrix(
  const Viewport::EditorCameraControllerState& controller,
  ViewportNavigationMode mode)
{
    const ml::vec3 forward = make_forward_direction(
      controller.pitch_radians,
      controller.yaw_radians);
    const ml::vec3 up = make_up_direction(
      controller.pitch_radians,
      controller.yaw_radians);

    if(mode == ViewportNavigationMode::Orbit)
    {
        const ml::vec3 eye =
          controller.orbit_target - forward * controller.orbit_distance;
        return ml::matrices::look_at(
          eye,
          controller.orbit_target,
          up);
    }

    return ml::matrices::look_at(
      controller.position,
      controller.position + forward,
      up);
}

}    // namespace

std::string_view to_string(EditorCameraView view)
{
    switch(view)
    {
    case EditorCameraView::Perspective:
        return "Perspective";
    case EditorCameraView::Top:
        return "Top";
    case EditorCameraView::Left:
        return "Left";
    case EditorCameraView::Front:
        return "Front";
    case EditorCameraView::Orthographic:
        return "Orthographic";
    }

    throw std::runtime_error{
      std::format(
        "Unknown EditorCameraView with value {}",
        static_cast<int>(view))};
}

Viewport::Viewport()
{
    sync_local_camera();
}

void Viewport::sync_local_camera()
{
    local_camera.set_projection_type(get_projection_type_for_view(editor_camera_view));
    local_camera.set_orthographic_height(editor_camera_controller.orthographic_height);
    local_camera.set_transform(
      make_view_matrix(
        editor_camera_controller,
        navigation_mode));
}

void Viewport::apply_editor_camera_view(
  EditorCameraView view,
  bool reset_controller)
{
    editor_camera_view = view;
    if(reset_controller)
    {
        reset_editor_camera_controller(editor_camera_controller, view);
    }
    sync_local_camera();
}

void Viewport::reset_editor_camera()
{
    reset_editor_camera_controller(
      editor_camera_controller,
      editor_camera_view);
    sync_local_camera();
}

void Viewport::set_editor_camera_view(EditorCameraView view)
{
    apply_editor_camera_view(view, true);
}

bool Viewport::is_local_camera_active(const Scene& scene) const
{
    return get_camera_type(scene) == ViewportCameraType::Local;
}

void Viewport::update_editor_camera(
  float delta_time,
  const ViewportEditorCameraInput& input)
{
    const ViewportNavigationMode mode = get_navigation_mode();
    if(static_cast<std::uint8_t>(mode) != last_navigation_mode)
    {
        synchronize_controller_to_navigation_mode(
          editor_camera_controller,
          mode);
        last_navigation_mode = static_cast<std::uint8_t>(mode);
    }

    if(!input.active || delta_time <= 0.f)
    {
        sync_local_camera();
        return;
    }

    const float look_sensitivity = 0.0025f;
    editor_camera_controller.yaw_radians += input.look_yaw * look_sensitivity;
    editor_camera_controller.pitch_radians += input.look_pitch * look_sensitivity;
    editor_camera_controller.pitch_radians = std::clamp(
      editor_camera_controller.pitch_radians,
      ml::to_radians(-90.f),
      ml::to_radians(90.f));

    const ProjectionType projection_type =
      get_projection_type_for_view(editor_camera_view);
    if(projection_type == ProjectionType::Orthographic
       && input.zoom_delta != 0.f)
    {
        const float zoom_step = std::max(
          0.5f,
          editor_camera_controller.orthographic_height * 0.1f);
        editor_camera_controller.orthographic_height = std::max(
          1.f,
          editor_camera_controller.orthographic_height - input.zoom_delta * zoom_step);
    }

    if(mode == ViewportNavigationMode::Orbit)
    {
        if(projection_type == ProjectionType::Perspective
           && input.zoom_delta != 0.f)
        {
            const float zoom_step = std::max(
              0.5f,
              editor_camera_controller.orbit_distance * 0.1f);
            editor_camera_controller.orbit_distance = std::max(
              1.f,
              editor_camera_controller.orbit_distance - input.zoom_delta * zoom_step);
        }
        sync_local_camera();
        return;
    }

    const float base_speed = input.fast_move ? 20.f : 8.f;
    const float move_distance = base_speed * delta_time;
    const ml::vec3 forward = make_forward_direction(
      editor_camera_controller.pitch_radians,
      editor_camera_controller.yaw_radians);
    const ml::vec3 right =
      make_right_direction(editor_camera_controller.yaw_radians);
    const ml::vec3 up = {0.f, 1.f, 0.f};

    editor_camera_controller.position += forward * (input.move_forward * move_distance);
    editor_camera_controller.position += right * (input.move_right * move_distance);
    editor_camera_controller.position += up * (input.move_up * move_distance);
    align_orbit_target_to_fps_view(editor_camera_controller);
    sync_local_camera();
}

#include <gtest/gtest.h>

#include "reflection/class_registry.h"
#include "scene/scene.h"
#include "viewport.h"

namespace
{

void ensure_scene_reflection_ready()
{
    static bool initialized = false;
    if(initialized)
    {
        return;
    }

    reflect::ReflectionSystem::allow_auto_registration(false);
    reflect::ReflectionSystem::process_pending_registrations();
    initialized = true;
}

void expect_matrix_near(
  const ml::mat4x4& actual,
  const ml::mat4x4& expected,
  float epsilon = 1e-5f)
{
    EXPECT_NEAR(actual.rows[0].x, expected.rows[0].x, epsilon);
    EXPECT_NEAR(actual.rows[0].y, expected.rows[0].y, epsilon);
    EXPECT_NEAR(actual.rows[0].z, expected.rows[0].z, epsilon);
    EXPECT_NEAR(actual.rows[0].w, expected.rows[0].w, epsilon);
    EXPECT_NEAR(actual.rows[1].x, expected.rows[1].x, epsilon);
    EXPECT_NEAR(actual.rows[1].y, expected.rows[1].y, epsilon);
    EXPECT_NEAR(actual.rows[1].z, expected.rows[1].z, epsilon);
    EXPECT_NEAR(actual.rows[1].w, expected.rows[1].w, epsilon);
    EXPECT_NEAR(actual.rows[2].x, expected.rows[2].x, epsilon);
    EXPECT_NEAR(actual.rows[2].y, expected.rows[2].y, epsilon);
    EXPECT_NEAR(actual.rows[2].z, expected.rows[2].z, epsilon);
    EXPECT_NEAR(actual.rows[2].w, expected.rows[2].w, epsilon);
    EXPECT_NEAR(actual.rows[3].x, expected.rows[3].x, epsilon);
    EXPECT_NEAR(actual.rows[3].y, expected.rows[3].y, epsilon);
    EXPECT_NEAR(actual.rows[3].z, expected.rows[3].z, epsilon);
    EXPECT_NEAR(actual.rows[3].w, expected.rows[3].w, epsilon);
}

}    // namespace

TEST(ViewportTests, DefaultsToLocalCamera)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Viewport viewport;

    EXPECT_EQ(viewport.get_scene_camera_id(), std::nullopt);
    EXPECT_EQ(&viewport.get_camera(scene), &viewport.get_local_camera());
    EXPECT_EQ(viewport.get_camera_type(scene), ViewportCameraType::Local);
    EXPECT_TRUE(viewport.is_editor_camera_view_active(
      scene,
      EditorCameraView::Perspective));
}

TEST(ViewportTests, UsesSceneCameraWhenPresent)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Viewport viewport;

    Camera* scene_camera = scene.create_object<Camera>();
    ASSERT_NE(scene_camera, nullptr);

    viewport.use_scene_camera(scene_camera->get_object_id());

    EXPECT_EQ(viewport.try_get_scene_camera(scene), scene_camera);
    EXPECT_EQ(&viewport.get_camera(scene), scene_camera);
    EXPECT_EQ(viewport.get_camera_type(scene), ViewportCameraType::Scene);
    EXPECT_TRUE(viewport.is_scene_camera_active(
      scene,
      scene_camera->get_object_id()));
    EXPECT_FALSE(viewport.is_editor_camera_view_active(
      scene,
      EditorCameraView::Perspective));
}

TEST(ViewportTests, FallsBackToLocalCameraWhenSceneCameraIsMissing)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Viewport viewport;

    viewport.use_scene_camera(make_object_id(9999));

    EXPECT_EQ(viewport.try_get_scene_camera(scene), nullptr);
    EXPECT_EQ(&viewport.get_camera(scene), &viewport.get_local_camera());
    EXPECT_EQ(viewport.get_camera_type(scene), ViewportCameraType::Local);
    EXPECT_TRUE(viewport.is_editor_camera_view_active(
      scene,
      EditorCameraView::Perspective));
}

TEST(ViewportTests, UseLocalCameraClearsSceneCameraSelection)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Viewport viewport;
    Camera* scene_camera = scene.create_object<Camera>();
    ASSERT_NE(scene_camera, nullptr);

    viewport.use_scene_camera(scene_camera->get_object_id());
    ASSERT_EQ(viewport.get_scene_camera_id(), scene_camera->get_object_id());

    viewport.use_local_camera();

    EXPECT_EQ(viewport.get_scene_camera_id(), std::nullopt);
    EXPECT_EQ(viewport.try_get_scene_camera(scene), nullptr);
}

TEST(ViewportTests, ResolutionIsClampedToAtLeastOne)
{
    Viewport viewport;

    viewport.set_resolution(0, -10);
    const ViewportResolution resolution = viewport.get_resolution();

    EXPECT_EQ(resolution.width, 1);
    EXPECT_EQ(resolution.height, 1);
    EXPECT_FLOAT_EQ(viewport.get_aspect_ratio(), 1.0f);
}

TEST(ViewportTests, AspectRatioUsesCurrentResolution)
{
    Viewport viewport;

    viewport.set_resolution(1920, 1080);

    const ViewportResolution resolution = viewport.get_resolution();
    EXPECT_EQ(resolution.width, 1920);
    EXPECT_EQ(resolution.height, 1080);
    EXPECT_FLOAT_EQ(viewport.get_aspect_ratio(), 1920.0f / 1080.0f);
}

TEST(ViewportTests, EditorCameraViewIsOwnedByViewportLocalCamera)
{
    Viewport viewport;

    EXPECT_EQ(viewport.get_editor_camera_view(), EditorCameraView::Perspective);
    EXPECT_EQ(
      viewport.get_local_camera().get_projection_type(),
      ProjectionType::Perspective);

    viewport.set_editor_camera_view(EditorCameraView::Orthographic);

    EXPECT_EQ(viewport.get_editor_camera_view(), EditorCameraView::Orthographic);
    EXPECT_EQ(
      viewport.get_local_camera().get_projection_type(),
      ProjectionType::Orthographic);
    EXPECT_STREQ(
      to_string(EditorCameraView::Orthographic).data(),
      "Orthographic");
}

TEST(ViewportTests, TopViewLooksStraightDownInOrbitMode)
{
    Viewport viewport;

    viewport.set_editor_camera_view(EditorCameraView::Top);

    const ml::mat4x4 expected = ml::matrices::look_at(
      {0.f, 40.f, 0.f},
      {0.f, 0.f, 0.f},
      {0.f, 0.f, -1.f});
    expect_matrix_near(
      viewport.get_local_camera().get_transform(),
      expected);
}

TEST(ViewportTests, TopViewRemainsStableWhenSwitchingToFpsMode)
{
    Viewport viewport;

    viewport.set_editor_camera_view(EditorCameraView::Top);
    viewport.set_navigation_mode(ViewportNavigationMode::Fps);
    viewport.update_editor_camera(1.f / 60.f, {});

    const ml::mat4x4 expected = ml::matrices::look_at(
      {0.f, 40.f, 0.f},
      {0.f, 39.f, 0.f},
      {0.f, 0.f, -1.f});
    expect_matrix_near(
      viewport.get_local_camera().get_transform(),
      expected);
}

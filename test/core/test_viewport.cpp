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

}    // namespace

TEST(ViewportTests, DefaultsToLocalCamera)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Viewport viewport;

    EXPECT_EQ(viewport.get_scene_camera_id(), std::nullopt);
    EXPECT_EQ(&viewport.get_camera(scene), &viewport.get_local_camera());
    EXPECT_EQ(viewport.get_camera_type(scene), ViewportCameraType::Local);
}

TEST(ViewportTests, UsesSceneCameraWhenPresent)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Viewport viewport;

    Camera* scene_camera = scene.add_object<Camera>();
    ASSERT_NE(scene_camera, nullptr);

    viewport.use_scene_camera(scene_camera->get_object_id());

    EXPECT_EQ(viewport.try_get_scene_camera(scene), scene_camera);
    EXPECT_EQ(&viewport.get_camera(scene), scene_camera);
    EXPECT_EQ(viewport.get_camera_type(scene), ViewportCameraType::Scene);
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
}

TEST(ViewportTests, UseLocalCameraClearsSceneCameraSelection)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Viewport viewport;
    Camera* scene_camera = scene.add_object<Camera>();
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

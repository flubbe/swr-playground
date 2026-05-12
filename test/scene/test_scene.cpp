#include <gtest/gtest.h>

#include "reflection/class_registry.h"
#include "scene/scene.h"

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

TEST(SceneTests, AddObjectSynchronizesObjectIndex)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Camera* camera = scene.add_object<Camera>();
    ASSERT_NE(camera, nullptr);

    const ObjectId id = camera->get_object_id();
    EXPECT_EQ(scene.find_object(id), camera);
    ASSERT_EQ(scene.get_objects_by_id().count(id), 1U);
    EXPECT_EQ(scene.get_objects_by_id().at(id), camera);
}

TEST(SceneTests, ClearRemovesObjectIndexAndSpinAnimations)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Camera* camera = scene.add_object<Camera>();
    ASSERT_NE(camera, nullptr);
    const ObjectId id = camera->get_object_id();

    scene.set_spin_animation(
      id,
      SpinAnimation{
        .translation = {1.f, 2.f, 3.f},
        .angular_speed = 2.f,
        .phase_offset = 5.f});
    ASSERT_EQ(scene.get_spin_animations().count(id), 1U);
    ASSERT_EQ(scene.get_objects_by_id().count(id), 1U);

    scene.clear();

    EXPECT_TRUE(scene.get_objects().empty());
    EXPECT_TRUE(scene.get_objects_by_id().empty());
    EXPECT_TRUE(scene.get_spin_animations().empty());
    EXPECT_EQ(scene.find_object(id), nullptr);
}

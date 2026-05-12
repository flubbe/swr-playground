#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "reflection/class_registry.h"
#include "scene/camera.h"

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

TEST(CameraReflectionTests, ExposesCameraPropertiesOnInstance)
{
    ensure_scene_reflection_ready();

    Camera camera;
    ASSERT_EQ(camera.get_class(), Camera::static_class());

    const auto& properties = camera.get_properties();
    std::vector<std::string_view> names;
    names.reserve(properties.size());
    for(const auto& property: properties)
    {
        ASSERT_NE(property, nullptr);
        names.push_back(property->get_name());
    }

    const auto has_name = [&](const std::string_view name)
    {
        return std::ranges::find(names, name) != names.end();
    };

    EXPECT_TRUE(has_name("object_id"));
    EXPECT_TRUE(has_name("name"));
    EXPECT_TRUE(has_name("transform"));
    EXPECT_TRUE(has_name("fov_y"));
    EXPECT_TRUE(has_name("near_plane"));
    EXPECT_TRUE(has_name("far_plane"));
}

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
    EXPECT_TRUE(has_name("orthographic_height"));
    EXPECT_TRUE(has_name("near_plane"));
    EXPECT_TRUE(has_name("far_plane"));
}

TEST(CameraReflectionTests, UsesOrthographicProjectionWhenRequested)
{
    Camera camera;
    camera.set_projection_mode(CameraProjectionMode::Orthographic);
    camera.set_orthographic_height(20.f);
    camera.update_projection_matrix(2.f);

    const float eps = 1e-5f;

    const ml::mat4x4 projection = camera.get_projection_matrix();
    EXPECT_NEAR(projection.rows[0].x, 0.05f, eps);
    EXPECT_NEAR(projection.rows[1].y, 0.1f, eps);
    EXPECT_NEAR(projection.rows[2].z, 2.f / 199.f, eps);
    EXPECT_NEAR(projection.rows[2].w, -201.f / 199.f, eps);
    EXPECT_NEAR(projection.rows[3].w, 1.f, eps);
}

#include <gtest/gtest.h>

#include "reflection/class_registry.h"
#include "scene/object.h"

namespace
{

void ensure_object_reflection_ready()
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

TEST(ObjectSnapshotTests, NoSnapshotMeansNoResetOperations)
{
    ensure_object_reflection_ready();
    Object object;

    EXPECT_FALSE(object.has_property_snapshot("name"));
    EXPECT_FALSE(object.reset_property_to_snapshot("name"));
    EXPECT_FALSE(object.reset_to_snapshot());
}

TEST(ObjectSnapshotTests, CaptureAndResetSinglePropertyRestoresBaseline)
{
    ensure_object_reflection_ready();
    Object object;
    object.set_name("captured_name");

    object.capture_snapshot();

    object.set_name("changed_name");

    EXPECT_TRUE(object.has_property_snapshot("name"));
    EXPECT_TRUE(object.reset_property_to_snapshot("name"));
    EXPECT_EQ(object.get_name(), "captured_name");
}

TEST(ObjectSnapshotTests, ResetToSnapshotSkipsReadOnlyProperties)
{
    ensure_object_reflection_ready();
    Object object;
    object.set_object_id(make_object_id(12));
    object.set_name("start");

    object.capture_snapshot();

    object.set_object_id(make_object_id(999));
    object.set_name("after");

    EXPECT_TRUE(object.reset_to_snapshot());
    EXPECT_EQ(object.get_name(), "start");
    EXPECT_EQ(object.get_object_id().value, 999u);
}

TEST(ObjectSnapshotTests, CaptureSnapshotUpdatesBaselineWhenReCaptured)
{
    ensure_object_reflection_ready();
    Object object;
    object.set_name("first");

    object.capture_snapshot();

    object.set_name("second");
    object.capture_snapshot();

    object.set_name("third");

    EXPECT_TRUE(object.reset_to_snapshot());
    EXPECT_EQ(object.get_name(), "second");
}

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "reflection/builtin_properties.h"
#include "reflection/class_registry.h"
#include "reflection/except.h"

class TestRoot : public reflect::ReflectRoot<TestRoot>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    int root_value{42};
    std::string root_name{"root"};
};

class TestChild : public reflect::Reflected<TestChild, TestRoot>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    bool enabled{true};
};

DECLARE_REFLECTION(Test, TestRoot);
DECLARE_REFLECTION(Test, TestChild);

class EmptyRoot : public reflect::ReflectRoot<EmptyRoot>
{
};

class EmptyChild : public reflect::Reflected<EmptyChild, EmptyRoot>
{
};

DECLARE_REFLECTION(Test, EmptyRoot);
DECLARE_REFLECTION(Test, EmptyChild);

void TestRoot::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&TestRoot::root_value>(
      class_info,
      "root_value",
      "Root Value",
      reflect::PropertyFlags::ReadOnly);
    reflect::register_property<&TestRoot::root_name>(
      class_info,
      "root_name",
      "Root Name");
}

void TestChild::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&TestChild::enabled>(
      class_info,
      "enabled",
      "Enabled");
}

DEFINE_REFLECTION(TestRoot);
DEFINE_REFLECTION(TestChild);
DEFINE_REFLECTION(EmptyRoot);
DEFINE_REFLECTION(EmptyChild);

namespace
{

void ensure_reflection_ready()
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

TEST(ReflectionSystemTests, ProcessPendingRequiresDisabledAutoRegistration)
{
    reflect::ReflectionSystem::allow_auto_registration(true);
    EXPECT_THROW(
      reflect::ReflectionSystem::process_pending_registrations(),
      std::runtime_error);
    reflect::ReflectionSystem::allow_auto_registration(false);
}

TEST(ReflectionSystemTests, FindsClassesAndSupportsHierarchyQueries)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* root_cls =
      reflect::ReflectionSystem::find_class<TestRoot>("Test.TestRoot");
    const reflect::ClassInfo* child_cls =
      reflect::ReflectionSystem::find_class<TestRoot>("Test.TestChild");

    ASSERT_NE(root_cls, nullptr);
    ASSERT_NE(child_cls, nullptr);
    EXPECT_TRUE(child_cls->is_a(root_cls));
    EXPECT_FALSE(root_cls->is_a(child_cls));

    TestRoot root;
    TestChild child;
    EXPECT_TRUE(root.is_a<TestRoot>());
    EXPECT_FALSE(root.is_a<TestChild>());
    EXPECT_TRUE(child.is_a<TestRoot>());
    EXPECT_TRUE(child.is_a<TestChild>());
}

TEST(ReflectionSystemTests, ConstructsRegisteredPropertyFromDescriptor)
{
    ensure_reflection_ready();

    TestChild child;
    const reflect::ClassInfo* child_cls = TestChild::static_class();
    ASSERT_NE(child_cls, nullptr);
    ASSERT_NE(child_cls->first_property, nullptr);

    const reflect::PropertyDescriptor* descriptor = child_cls->first_property.get();
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->construct, nullptr);

    std::unique_ptr<reflect::Property> property = descriptor->construct(
      &child,
      descriptor->name,
      descriptor->label,
      descriptor->flags);

    ASSERT_NE(property, nullptr);
    ASSERT_TRUE(property->is_type<reflect::BoolProperty>());
    reflect::BoolProperty& bool_property = property->as<reflect::BoolProperty>();
    EXPECT_TRUE(bool_property.get_value());
    EXPECT_TRUE(bool_property.set_value(false));
    EXPECT_FALSE(child.enabled);
}

TEST(ReflectionSystemTests, FindsPropertyDescriptorsByInternalName)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* root_cls = TestRoot::static_class();
    ASSERT_NE(root_cls, nullptr);

    const reflect::PropertyDescriptor* root_name_descriptor =
      root_cls->find_property("root_name");
    ASSERT_NE(root_name_descriptor, nullptr);
    EXPECT_EQ(root_name_descriptor->name, "root_name");
    EXPECT_EQ(root_name_descriptor->label, "Root Name");

    EXPECT_EQ(root_cls->find_property("missing_property"), nullptr);

    reflect::ClassInfo* child_cls = TestChild::static_class();
    ASSERT_NE(child_cls, nullptr);

    reflect::PropertyDescriptor* enabled_descriptor =
      child_cls->find_property("enabled");
    ASSERT_NE(enabled_descriptor, nullptr);
    EXPECT_EQ(enabled_descriptor->name, "enabled");
}

TEST(ReflectionSystemTests, UsesDefaultNoOpPropertyRegistrationWhenNotDefined)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* root_cls =
      reflect::ReflectionSystem::find_class<EmptyRoot>("Test.EmptyRoot");
    const reflect::ClassInfo* child_cls =
      reflect::ReflectionSystem::find_class<EmptyRoot>("Test.EmptyChild");

    ASSERT_NE(root_cls, nullptr);
    ASSERT_NE(child_cls, nullptr);
    EXPECT_EQ(root_cls->first_property, nullptr);
    EXPECT_EQ(child_cls->first_property, nullptr);
    EXPECT_TRUE(child_cls->is_a(root_cls));
}

TEST(ReflectionSystemTests, ConstructDescriptorThrowsOnNullObject)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* child_cls = TestChild::static_class();
    ASSERT_NE(child_cls, nullptr);
    ASSERT_NE(child_cls->first_property, nullptr);

    const reflect::PropertyDescriptor* descriptor = child_cls->first_property.get();
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->construct, nullptr);

    EXPECT_THROW(
      descriptor->construct(
        nullptr,
        descriptor->name,
        descriptor->label,
        descriptor->flags),
      reflect::instance_error);
}

TEST(BuiltinPropertyTests, ConstructorsRejectNullValuePointers)
{
    EXPECT_THROW(
      reflect::IntProperty("int_value", "Int Value", nullptr),
      std::invalid_argument);
    EXPECT_THROW(
      reflect::UIntProperty("uint_value", "UInt Value", nullptr),
      std::invalid_argument);
    EXPECT_THROW(
      reflect::FloatProperty("float_value", "Float Value", nullptr),
      std::invalid_argument);
    EXPECT_THROW(
      reflect::BoolProperty("bool_value", "Bool Value", nullptr),
      std::invalid_argument);
    EXPECT_THROW(
      reflect::StringProperty("string_value", "String Value", nullptr),
      std::invalid_argument);
}

TEST(BuiltinPropertyTests, ReadOnlyIntPropertyPreventsWrites)
{
    int value = 7;
    reflect::IntProperty property(
      "value",
      "Value",
      &value,
      reflect::PropertyFlags::ReadOnly);

    EXPECT_FALSE(property.set_value(11));
    EXPECT_EQ(value, 7);
}

TEST(BuiltinPropertyTests, TypeTagCastsWorkForPropertyBase)
{
    int value = 3;
    reflect::IntProperty int_property("value", "Value", &value);
    reflect::Property& property = int_property;

    EXPECT_TRUE(property.is_type<reflect::IntProperty>());
    EXPECT_NE(property.try_as<reflect::IntProperty>(), nullptr);
    EXPECT_EQ(property.try_as<reflect::FloatProperty>(), nullptr);
}

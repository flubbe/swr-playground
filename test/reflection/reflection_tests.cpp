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

DECLARE_REFLECTION(Test, TestRoot);
DEFINE_REFLECTION(TestRoot);

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

class TestChild : public reflect::Reflected<TestChild, TestRoot>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    bool enabled{true};
};

DECLARE_REFLECTION(Test, TestChild);
DEFINE_REFLECTION(TestChild);

void TestChild::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&TestChild::enabled>(
      class_info,
      "enabled",
      "Enabled");
}

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

TEST(ReflectionSystemTests, StaticClassReturnsStablePointer)
{
    ensure_reflection_ready();

    EXPECT_EQ(TestRoot::static_class(), TestRoot::static_class());
    EXPECT_EQ(TestChild::static_class(), TestChild::static_class());
}

TEST(ReflectionSystemTests, InstanceClassMatchesStaticClass)
{
    ensure_reflection_ready();

    TestRoot root;
    TestChild child;

    EXPECT_EQ(root.get_class(), TestRoot::static_class());
    EXPECT_EQ(child.get_class(), TestChild::static_class());
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

TEST(ReflectionSystemTests, DerivedClassFirstPropertyContainsOnlyLocalProperties)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = TestChild::static_class();

    ASSERT_NE(cls, nullptr);
    ASSERT_NE(cls->first_property, nullptr);

    EXPECT_EQ(cls->first_property->name, "enabled");
    EXPECT_EQ(cls->first_property->next, nullptr);
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

TEST(ReflectionSystemTests, FindsInheritedPropertyDescriptorsByInternalName)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* child_cls =
      reflect::ReflectionSystem::find_class<TestRoot>("Test.TestChild");

    ASSERT_NE(child_cls, nullptr);

    const reflect::PropertyDescriptor* inherited =
      child_cls->find_property("root_value");

    ASSERT_NE(inherited, nullptr);
    EXPECT_EQ(inherited, TestRoot::static_class()->first_property.get());
    EXPECT_EQ(inherited->name, "root_value");
    EXPECT_EQ(inherited->label, "Root Value");
}

TEST(ReflectionSystemTests, InheritedDescriptorConstructsPropertyForDerivedInstance)
{
    ensure_reflection_ready();

    TestChild child;
    const reflect::ClassInfo* child_cls =
      reflect::ReflectionSystem::find_class<TestRoot>("Test.TestChild");

    ASSERT_NE(child_cls, nullptr);
    ASSERT_TRUE(child_cls->is_a(TestRoot::static_class()));

    const reflect::PropertyDescriptor* descriptor =
      child_cls->find_property("root_name");

    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->construct, nullptr);

    auto property = descriptor->construct(
      &child,
      descriptor->name,
      descriptor->label,
      descriptor->flags);

    ASSERT_NE(property, nullptr);
    ASSERT_TRUE(property->is_type<reflect::StringProperty>());

    auto& string_property = property->as<reflect::StringProperty>();
    EXPECT_EQ(string_property.get_value(), "root");
}

TEST(ReflectionSystemTests, PreservesRegistrationOrderWithinClass)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = TestRoot::static_class();

    ASSERT_NE(cls, nullptr);
    ASSERT_NE(cls->first_property, nullptr);

    EXPECT_EQ(cls->first_property->name, "root_value");
    ASSERT_NE(cls->first_property->next, nullptr);
    EXPECT_EQ(cls->first_property->next->name, "root_name");
    EXPECT_EQ(cls->first_property->next->next, nullptr);
}

class EmptyRoot : public reflect::ReflectRoot<EmptyRoot>
{
};

DECLARE_REFLECTION(Test, EmptyRoot);
DEFINE_REFLECTION(EmptyRoot);

class EmptyChild : public reflect::Reflected<EmptyChild, EmptyRoot>
{
public:
    static void register_properties(reflect::ClassInfo&);
};

DECLARE_REFLECTION(Test, EmptyChild);
DEFINE_REFLECTION(EmptyChild);

void EmptyChild::register_properties(reflect::ClassInfo&)
{
}

TEST(ReflectionSystemTests, SupportsExplicitEmptyPropertyRegistration)
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

TEST(ReflectionSystemTests, RootCanOmitPropertyRegistration)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* root_cls = EmptyRoot::static_class();

    ASSERT_NE(root_cls, nullptr);
    EXPECT_EQ(root_cls->first_property, nullptr);
}

class EmptyRegisteredChild : public reflect::Reflected<EmptyRegisteredChild, TestRoot>
{
public:
    static void register_properties(reflect::ClassInfo&);
};

DECLARE_REFLECTION(Test, EmptyRegisteredChild);
DEFINE_REFLECTION(EmptyRegisteredChild);

void EmptyRegisteredChild::register_properties(reflect::ClassInfo&)
{
}

TEST(ReflectionSystemTests, EmptyChildFindsInheritedPropertiesWithoutCopyingThem)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = EmptyRegisteredChild::static_class();

    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->first_property, nullptr);
    EXPECT_NE(cls->find_property("root_value"), nullptr);
    EXPECT_NE(cls->find_property("root_name"), nullptr);
    EXPECT_EQ(cls->find_property("missing_property"), nullptr);
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

class ShadowChild : public reflect::Reflected<ShadowChild, TestRoot>
{
public:
    static void register_properties(reflect::ClassInfo&);

    int root_value{100};
};

DECLARE_REFLECTION(Test, ShadowChild);
DEFINE_REFLECTION(ShadowChild);

void ShadowChild::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&ShadowChild::root_value>(
      class_info,
      "root_value",
      "Shadowed Root Value");
}

TEST(ReflectionSystemTests, FindPropertyPrefersMostDerivedDescriptor)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = ShadowChild::static_class();
    ASSERT_NE(cls, nullptr);

    const reflect::PropertyDescriptor* descriptor =
      cls->find_property("root_value");

    ASSERT_NE(descriptor, nullptr);
    EXPECT_EQ(descriptor->label, "Shadowed Root Value");
    EXPECT_EQ(descriptor, cls->first_property.get());
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

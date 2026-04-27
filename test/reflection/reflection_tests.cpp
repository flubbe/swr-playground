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

struct RuntimeRootA
{
};

struct RuntimeRootB
{
};

struct RuntimeRootDuplicate
{
};

struct RuntimeRootUnregister
{
};

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

TEST(ReflectionSystemTests, SupportsSameQualifiedNameAcrossDifferentRoots)
{
    ensure_reflection_ready();

    reflect::ClassInfo class_a_storage{};
    reflect::ClassInfo class_b_storage{};

    reflect::detail::PendingClassRegistration reg_a{
      .module_name = "RuntimeIso",
      .name = "SharedName",
      .size = sizeof(int),
      .storage = &class_a_storage,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootA>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassRegistration reg_b{
      .module_name = "RuntimeIso",
      .name = "SharedName",
      .size = sizeof(int),
      .storage = &class_b_storage,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootB>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};

    reflect::detail::PendingClassNode node_a{
      .reg = &reg_a,
      .next = nullptr};
    reflect::detail::PendingClassNode node_b{
      .reg = &reg_b,
      .next = nullptr};

    reflect::detail::AutoClassRegistrar registrar_a{&node_a};
    reflect::detail::AutoClassRegistrar registrar_b{&node_b};

    EXPECT_NO_THROW(reflect::ReflectionSystem::process_pending_registrations());

    const reflect::ClassInfo* class_a = reflect::ReflectionSystem::find_class(
      "RuntimeIso.SharedName",
      reflect::detail::root_type_tag<RuntimeRootA>());
    const reflect::ClassInfo* class_b = reflect::ReflectionSystem::find_class(
      "RuntimeIso.SharedName",
      reflect::detail::root_type_tag<RuntimeRootB>());

    EXPECT_EQ(class_a, &class_a_storage);
    EXPECT_EQ(class_b, &class_b_storage);

    EXPECT_TRUE(reflect::ReflectionSystem::unregister_class(
      "RuntimeIso.SharedName",
      reflect::detail::root_type_tag<RuntimeRootA>()));
    EXPECT_TRUE(reflect::ReflectionSystem::unregister_class(
      "RuntimeIso.SharedName",
      reflect::detail::root_type_tag<RuntimeRootB>()));
}

TEST(ReflectionSystemTests, RejectsDuplicateQualifiedNameWithinSameRoot)
{
    ensure_reflection_ready();

    reflect::ClassInfo first_storage{};
    reflect::ClassInfo second_storage{};

    reflect::detail::PendingClassRegistration first_reg{
      .module_name = "RuntimeDup",
      .name = "SameName",
      .size = sizeof(int),
      .storage = &first_storage,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootDuplicate>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassRegistration second_reg{
      .module_name = "RuntimeDup",
      .name = "SameName",
      .size = sizeof(int),
      .storage = &second_storage,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootDuplicate>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};

    reflect::detail::PendingClassNode first_node{
      .reg = &first_reg,
      .next = nullptr};
    reflect::detail::PendingClassNode second_node{
      .reg = &second_reg,
      .next = nullptr};

    reflect::detail::AutoClassRegistrar first_registrar{&first_node};
    reflect::detail::AutoClassRegistrar second_registrar{&second_node};

    EXPECT_THROW(
      reflect::ReflectionSystem::process_pending_registrations(),
      std::runtime_error);

    // The first processed duplicate may already be registered before the throw.
    reflect::ReflectionSystem::unregister_class(
      "RuntimeDup.SameName",
      reflect::detail::root_type_tag<RuntimeRootDuplicate>());
}

TEST(ReflectionSystemTests, UnregisterClassAndModuleRemoveExpectedEntries)
{
    ensure_reflection_ready();

    reflect::ClassInfo class_a_storage{};
    reflect::ClassInfo class_b_storage{};

    reflect::detail::PendingClassRegistration reg_a{
      .module_name = "RuntimeUnregister",
      .name = "ClassA",
      .size = sizeof(int),
      .storage = &class_a_storage,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootUnregister>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassRegistration reg_b{
      .module_name = "RuntimeUnregister",
      .name = "ClassB",
      .size = sizeof(int),
      .storage = &class_b_storage,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootUnregister>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};

    reflect::detail::PendingClassNode node_a{
      .reg = &reg_a,
      .next = nullptr};
    reflect::detail::PendingClassNode node_b{
      .reg = &reg_b,
      .next = nullptr};

    reflect::detail::AutoClassRegistrar registrar_a{&node_a};
    reflect::detail::AutoClassRegistrar registrar_b{&node_b};

    EXPECT_NO_THROW(reflect::ReflectionSystem::process_pending_registrations());

    EXPECT_NE(
      reflect::ReflectionSystem::find_class(
        "RuntimeUnregister.ClassA",
        reflect::detail::root_type_tag<RuntimeRootUnregister>()),
      nullptr);
    EXPECT_NE(
      reflect::ReflectionSystem::find_class(
        "RuntimeUnregister.ClassB",
        reflect::detail::root_type_tag<RuntimeRootUnregister>()),
      nullptr);

    EXPECT_TRUE(reflect::ReflectionSystem::unregister_class(
      "RuntimeUnregister.ClassA",
      reflect::detail::root_type_tag<RuntimeRootUnregister>()));
    EXPECT_FALSE(reflect::ReflectionSystem::unregister_class(
      "RuntimeUnregister.ClassA",
      reflect::detail::root_type_tag<RuntimeRootUnregister>()));

    EXPECT_EQ(reflect::ReflectionSystem::unregister_module("RuntimeUnregister"), 1);
    EXPECT_EQ(reflect::ReflectionSystem::unregister_module("RuntimeUnregister"), 0);

    EXPECT_EQ(
      reflect::ReflectionSystem::find_class(
        "RuntimeUnregister.ClassB",
        reflect::detail::root_type_tag<RuntimeRootUnregister>()),
      nullptr);
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

TEST(ReflectionSystemTests, LazySuperResolutionMatchesBetweenStaticAndInstanceClass)
{
    ensure_reflection_ready();

    TestChild child;
    const reflect::ClassInfo* static_child_class = TestChild::static_class();
    const reflect::ClassInfo* instance_child_class = child.get_class();

    ASSERT_NE(static_child_class, nullptr);
    ASSERT_NE(instance_child_class, nullptr);

    const reflect::ClassInfo* static_super = static_child_class->get_super();
    const reflect::ClassInfo* instance_super = instance_child_class->get_super();

    EXPECT_EQ(static_super, TestRoot::static_class());
    EXPECT_EQ(instance_super, TestRoot::static_class());
    EXPECT_EQ(static_super, instance_super);
}

TEST(ReflectionSystemTests, FactoryAndDestroyCreateAndDestroyDerivedInstance)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* child_cls = TestChild::static_class();
    ASSERT_NE(child_cls, nullptr);
    ASSERT_NE(child_cls->factory, nullptr);
    ASSERT_NE(child_cls->destroy, nullptr);

    void* raw_instance = child_cls->factory();
    ASSERT_NE(raw_instance, nullptr);

    TestRoot* root_ptr = static_cast<TestRoot*>(raw_instance);
    ASSERT_NE(root_ptr, nullptr);
    EXPECT_TRUE(root_ptr->is_a(TestChild::static_class()));
    EXPECT_EQ(root_ptr->get_class(), TestChild::static_class());

    child_cls->destroy(raw_instance);
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

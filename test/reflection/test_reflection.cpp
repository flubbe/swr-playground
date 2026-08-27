#include <stdexcept>
#include <string>
#include <functional>
#include <cstdint>

#include <gtest/gtest.h>

#include "containers/memory.h"
#include "reflection/builtin_properties.h"
#include "reflection/class_registry.h"
#include "reflection/construct.h"
#include "reflection/except.h"

class TestRoot : public reflect::ReflectRoot<TestRoot>
{
protected:
    std::size_t* destructor_calls{nullptr};

public:
    virtual ~TestRoot()
    {
        if(destructor_calls != nullptr)
        {
            ++(*destructor_calls);
        }
    }

    static void register_properties(reflect::ClassInfo& class_info);
    void init(std::size_t* destructor_calls)
    {
        this->destructor_calls = destructor_calls;
    }

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
    virtual ~TestChild()
    {
        if(destructor_calls != nullptr)
        {
            ++(*destructor_calls);
        }
    }

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

class TestGrandChild : public reflect::Reflected<TestGrandChild, TestChild>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);
};

DECLARE_REFLECTION(Test, TestGrandChild);
DEFINE_REFLECTION(TestGrandChild);

void TestGrandChild::register_properties([[maybe_unused]] reflect::ClassInfo& class_info)
{
}

class NonReflectedBase
{
public:
    virtual ~NonReflectedBase() = default;
    std::uint32_t sentinel{0xA5A5A5A5u};
};

class OffsetChild : public NonReflectedBase
, public reflect::Reflected<OffsetChild, TestRoot>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    bool local_flag{false};
};

DECLARE_REFLECTION(Test, OffsetChild);
DEFINE_REFLECTION(OffsetChild);

void OffsetChild::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&OffsetChild::local_flag>(
      class_info,
      "local_flag",
      "Local Flag");
}

class ConstrainedChild : public reflect::Reflected<ConstrainedChild, TestRoot>
{
public:
    struct CustomConstraint : reflect::PropertyConstraint
    {
        int marker{0};

        const void* get_type_tag() const noexcept override
        {
            return reflect::detail::type_tag<CustomConstraint>();
        }
    };

    static void register_properties(reflect::ClassInfo& class_info);

    int constrained_value{5};
};

DECLARE_REFLECTION(Test, ConstrainedChild);
DEFINE_REFLECTION(ConstrainedChild);

void ConstrainedChild::register_properties(reflect::ClassInfo& class_info)
{
    auto custom = std::make_shared<CustomConstraint>();
    custom->marker = 77;
    reflect::register_property<&ConstrainedChild::constrained_value>(
      class_info,
      "constrained_value",
      "Constrained Value",
      reflect::PropertyFlags::None,
      std::static_pointer_cast<const reflect::PropertyConstraint>(custom));
}

class RangeConstrainedChild : public reflect::Reflected<RangeConstrainedChild, TestRoot>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    int constrained_teeth{10};
};

DECLARE_REFLECTION(Test, RangeConstrainedChild);
DEFINE_REFLECTION(RangeConstrainedChild);

void RangeConstrainedChild::register_properties(reflect::ClassInfo& class_info)
{
    reflect::RangeConstraint<int> range{};
    range.min = 5;
    range.max = 50;
    range.step = 1;
    range.clamp = true;

    reflect::register_property<&RangeConstrainedChild::constrained_teeth>(
      class_info,
      "constrained_teeth",
      "Constrained Teeth",
      reflect::PropertyFlags::None,
      range);
}

class DefaultedChild : public reflect::Reflected<DefaultedChild, TestRoot>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    int defaulted_value{11};
    float ranged_defaulted_value{0.75f};
};

DECLARE_REFLECTION(Test, DefaultedChild);
DEFINE_REFLECTION(DefaultedChild);

void DefaultedChild::register_properties(reflect::ClassInfo& class_info)
{
    reflect::register_property<&DefaultedChild::defaulted_value>(
      class_info,
      "defaulted_value",
      "Defaulted Value",
      reflect::PropertyFlags::None,
      reflect::default_of(42));

    reflect::RangeConstraint<float> range{};
    range.min = 0.0f;
    range.max = 1.0f;
    range.step = 0.05f;
    range.clamp = true;

    reflect::register_property<&DefaultedChild::ranged_defaulted_value>(
      class_info,
      "ranged_defaulted_value",
      "Ranged Defaulted Value",
      reflect::PropertyFlags::None,
      range,
      0.5f);
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

struct RuntimeRootSuper
{
};

struct RuntimeRootInvalid
{
};

struct RuntimeRootClear
{
};

const reflect::ClassInfo* g_cycle_self = nullptr;
const reflect::ClassInfo* g_cycle_a = nullptr;
const reflect::ClassInfo* g_cycle_b = nullptr;
const reflect::ClassInfo* g_chain_super_a = nullptr;
const reflect::ClassInfo* g_chain_super_b = nullptr;
int g_throwing_resolver_calls = 0;

const reflect::ClassInfo* resolve_cycle_self()
{
    return g_cycle_self;
}

const reflect::ClassInfo* resolve_cycle_a()
{
    return g_cycle_b;
}

const reflect::ClassInfo* resolve_cycle_b()
{
    return g_cycle_a;
}

const reflect::ClassInfo* resolve_throw_once()
{
    ++g_throwing_resolver_calls;
    throw std::runtime_error{"Synthetic resolver failure"};
}

const reflect::ClassInfo* resolve_chain_super_a()
{
    return g_chain_super_a;
}

const reflect::ClassInfo* resolve_chain_super_b()
{
    return g_chain_super_b;
}

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

TEST(ReflectionSystemTests, PendingNeedsAutoRegOff)
{
    reflect::ReflectionSystem::allow_auto_registration(true);
    EXPECT_THROW(
      reflect::ReflectionSystem::process_pending_registrations(),
      std::runtime_error);
    reflect::ReflectionSystem::allow_auto_registration(false);
}

TEST(ReflectionSystemTests, AllowsSameNameAcrossRoots)
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

TEST(ReflectionSystemTests, RejectsDuplicateNameInRoot)
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

TEST(ReflectionSystemTests, ResolvesSuperChain)
{
    ensure_reflection_ready();

    reflect::ClassInfo class_a_storage{};
    reflect::ClassInfo class_b_storage{};
    reflect::ClassInfo class_c_storage{};
    g_chain_super_a = &class_a_storage;
    g_chain_super_b = &class_b_storage;

    reflect::detail::PendingClassRegistration reg_a{
      .module_name = "RuntimeSuper",
      .name = "A",
      .size = sizeof(int),
      .storage = &class_a_storage,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootSuper>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassRegistration reg_b{
      .module_name = "RuntimeSuper",
      .name = "B",
      .size = sizeof(int),
      .storage = &class_b_storage,
      .resolve_super = &resolve_chain_super_a,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootSuper>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassRegistration reg_c{
      .module_name = "RuntimeSuper",
      .name = "C",
      .size = sizeof(int),
      .storage = &class_c_storage,
      .resolve_super = &resolve_chain_super_b,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootSuper>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};

    reflect::detail::PendingClassNode node_a{.reg = &reg_a, .next = nullptr};
    reflect::detail::PendingClassNode node_b{.reg = &reg_b, .next = nullptr};
    reflect::detail::PendingClassNode node_c{.reg = &reg_c, .next = nullptr};

    reflect::detail::AutoClassRegistrar registrar_a{&node_a};
    reflect::detail::AutoClassRegistrar registrar_b{&node_b};
    reflect::detail::AutoClassRegistrar registrar_c{&node_c};

    EXPECT_NO_THROW(reflect::ReflectionSystem::process_pending_registrations());

    EXPECT_EQ(class_a_storage.get_super(), nullptr);
    EXPECT_EQ(class_b_storage.get_super(), &class_a_storage);
    EXPECT_EQ(class_c_storage.get_super(), &class_b_storage);

    reflect::ReflectionSystem::unregister_module("RuntimeSuper");
    g_chain_super_a = nullptr;
    g_chain_super_b = nullptr;
}

TEST(ReflectionSystemTests, RejectsDirectSuperCycle)
{
    ensure_reflection_ready();

    reflect::ClassInfo class_storage{};

    g_cycle_self = &class_storage;
    reflect::detail::PendingClassRegistration reg{
      .module_name = "RuntimeCycle",
      .name = "Self",
      .size = sizeof(int),
      .storage = &class_storage,
      .resolve_super = &resolve_cycle_self,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootSuper>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassNode node{.reg = &reg, .next = nullptr};
    reflect::detail::AutoClassRegistrar registrar{&node};

    EXPECT_THROW(
      reflect::ReflectionSystem::process_pending_registrations(),
      std::runtime_error);

    g_cycle_self = nullptr;
    reflect::ReflectionSystem::unregister_module("RuntimeCycle");
}

TEST(ReflectionSystemTests, RejectsIndirectSuperCycle)
{
    ensure_reflection_ready();

    reflect::ClassInfo class_a_storage{};
    reflect::ClassInfo class_b_storage{};

    g_cycle_a = &class_a_storage;
    g_cycle_b = &class_b_storage;

    reflect::detail::PendingClassRegistration reg_a{
      .module_name = "RuntimeCycle2",
      .name = "A",
      .size = sizeof(int),
      .storage = &class_a_storage,
      .resolve_super = &resolve_cycle_a,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootSuper>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassRegistration reg_b{
      .module_name = "RuntimeCycle2",
      .name = "B",
      .size = sizeof(int),
      .storage = &class_b_storage,
      .resolve_super = &resolve_cycle_b,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootSuper>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassNode node_a{.reg = &reg_a, .next = nullptr};
    reflect::detail::PendingClassNode node_b{.reg = &reg_b, .next = nullptr};
    reflect::detail::AutoClassRegistrar registrar_a{&node_a};
    reflect::detail::AutoClassRegistrar registrar_b{&node_b};

    EXPECT_THROW(
      reflect::ReflectionSystem::process_pending_registrations(),
      std::runtime_error);

    g_cycle_a = nullptr;
    g_cycle_b = nullptr;
    reflect::ReflectionSystem::unregister_module("RuntimeCycle2");
}

TEST(ReflectionSystemTests, PropagatesSuperResolverError)
{
    ensure_reflection_ready();

    reflect::ClassInfo class_storage{};
    g_throwing_resolver_calls = 0;

    reflect::detail::PendingClassRegistration reg{
      .module_name = "RuntimeThrow",
      .name = "Throwing",
      .size = sizeof(int),
      .storage = &class_storage,
      .resolve_super = &resolve_throw_once,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootSuper>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassNode node{.reg = &reg, .next = nullptr};
    reflect::detail::AutoClassRegistrar registrar{&node};

    EXPECT_THROW(
      reflect::ReflectionSystem::process_pending_registrations(),
      std::runtime_error);
    EXPECT_EQ(g_throwing_resolver_calls, 1);

    reflect::ReflectionSystem::unregister_module("RuntimeThrow");
}

TEST(ReflectionSystemTests, AllowsRegisteredResolvedSuper)
{
    ensure_reflection_ready();

    reflect::ClassInfo child_storage{};
    reflect::detail::PendingClassRegistration child_reg{
      .module_name = "RuntimeSuperRef",
      .name = "Child",
      .size = sizeof(int),
      .storage = &child_storage,
      .resolve_super = []() -> const reflect::ClassInfo*
      { return TestRoot::static_class(); },
      .root_tag = reflect::detail::root_type_tag<TestRoot>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassNode child_node{.reg = &child_reg, .next = nullptr};
    reflect::detail::AutoClassRegistrar registrar{&child_node};

    EXPECT_NO_THROW(reflect::ReflectionSystem::process_pending_registrations());
    EXPECT_EQ(child_storage.get_super(), TestRoot::static_class());

    reflect::ReflectionSystem::unregister_module("RuntimeSuperRef");
}

TEST(ReflectionSystemTests, UnregisterRemovesEntries)
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

TEST(ReflectionSystemTests, RejectsPendingWithNullReg)
{
    ensure_reflection_ready();

    reflect::detail::PendingClassNode node{
      .reg = nullptr,
      .next = nullptr};
    reflect::detail::AutoClassRegistrar registrar{&node};

    EXPECT_THROW(
      reflect::ReflectionSystem::process_pending_registrations(),
      std::runtime_error);
}

TEST(ReflectionSystemTests, RejectsPendingWithNullStorage)
{
    ensure_reflection_ready();

    reflect::detail::PendingClassRegistration reg{
      .module_name = "RuntimeInvalid",
      .name = "NoStorage",
      .size = sizeof(int),
      .storage = nullptr,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootInvalid>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassNode node{.reg = &reg, .next = nullptr};
    reflect::detail::AutoClassRegistrar registrar{&node};

    EXPECT_THROW(
      reflect::ReflectionSystem::process_pending_registrations(),
      std::runtime_error);
}

TEST(ReflectionSystemTests, FindClassWrongRootReturnsNull)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* found = reflect::ReflectionSystem::find_class(
      "Test.TestRoot",
      reflect::detail::root_type_tag<RuntimeRootA>());
    EXPECT_EQ(found, nullptr);
}

TEST(ReflectionSystemTests, RegisteredClassesAreSorted)
{
    ensure_reflection_ready();

    swr::vector<const reflect::ClassInfo*> classes;
    reflect::ReflectionSystem::get_registered_classes(classes);
    ASSERT_FALSE(classes.empty());

    for(std::size_t i = 1; i < classes.size(); ++i)
    {
        ASSERT_NE(classes[i - 1], nullptr);
        ASSERT_NE(classes[i], nullptr);
        const auto& prev = classes[i - 1];
        const auto& curr = classes[i];

        if(prev->qualified_name == curr->qualified_name)
        {
            EXPECT_TRUE(std::less<>{}(prev->root_tag, curr->root_tag));
        }
        else
        {
            EXPECT_LT(prev->qualified_name, curr->qualified_name);
        }
    }
}

TEST(ReflectionSystemTests, StaticClassIsStable)
{
    ensure_reflection_ready();

    EXPECT_EQ(TestRoot::static_class(), TestRoot::static_class());
    EXPECT_EQ(TestChild::static_class(), TestChild::static_class());
}

TEST(ReflectionSystemTests, InstanceClassMatchesStatic)
{
    ensure_reflection_ready();

    TestRoot root;
    TestChild child;

    EXPECT_EQ(root.get_class(), TestRoot::static_class());
    EXPECT_EQ(child.get_class(), TestChild::static_class());
}

TEST(ReflectionSystemTests, SuperMatchesStaticAndInstance)
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

TEST(ReflectionSystemTests, FactoryDestroyDerivedInstance)
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

TEST(ReflectionSystemTests, FindClassAndIsASupport)
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

TEST(ReflectionSystemTests, ConstructsPropertyFromDescriptor)
{
    ensure_reflection_ready();

    TestChild child;
    const reflect::ClassInfo* child_cls = TestChild::static_class();
    ASSERT_NE(child_cls, nullptr);
    ASSERT_NE(child_cls->first_property, nullptr);

    const reflect::PropertyDescriptor* descriptor = child_cls->first_property.get();
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->construct, nullptr);

    swr::unique_ptr<reflect::Property> property = descriptor->construct(
      &child,
      descriptor->name,
      descriptor->label,
      descriptor->flags,
      descriptor->constraint);

    ASSERT_NE(property, nullptr);
    ASSERT_TRUE(property->is_type<reflect::BoolProperty>());
    EXPECT_EQ(property->get_size(), sizeof(bool));
    EXPECT_EQ(property->get_alignment(), alignof(bool));
    EXPECT_EQ(
      property->get_offset(),
      static_cast<std::size_t>(
        reinterpret_cast<std::uintptr_t>(std::addressof(child.enabled))
        - reinterpret_cast<std::uintptr_t>(std::addressof(child))));
    reflect::BoolProperty& bool_property = property->as<reflect::BoolProperty>();
    EXPECT_TRUE(bool_property.get_value(&child.enabled));
    EXPECT_TRUE(bool_property.set_value(&child.enabled, false));
    EXPECT_FALSE(child.enabled);
}

TEST(ReflectionSystemTests, DerivedFirstPropertyIsLocalOnly)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = TestChild::static_class();

    ASSERT_NE(cls, nullptr);
    ASSERT_NE(cls->first_property, nullptr);

    EXPECT_EQ(cls->first_property->name, "enabled");
    EXPECT_EQ(cls->first_property->next, nullptr);
}

TEST(ReflectionSystemTests, FindsDescriptorByName)
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

TEST(ReflectionSystemTests, FindsInheritedDescriptorByName)
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

TEST(ReflectionSystemTests, InheritedDescriptorConstructsDerived)
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
      descriptor->flags,
      descriptor->constraint);

    ASSERT_NE(property, nullptr);
    ASSERT_TRUE(property->is_type<reflect::StringProperty>());
    EXPECT_EQ(property->get_size(), sizeof(std::string));
    EXPECT_EQ(property->get_alignment(), alignof(std::string));
    EXPECT_EQ(
      property->get_offset(),
      static_cast<std::size_t>(
        reinterpret_cast<std::uintptr_t>(std::addressof(child.root_name))
        - reinterpret_cast<std::uintptr_t>(std::addressof(child))));

    auto& string_property = property->as<reflect::StringProperty>();
    auto root_name_address = reinterpret_cast<std::uintptr_t>(&child) + property->get_offset();
    EXPECT_EQ(string_property.get_value(
                reinterpret_cast<const void*>(root_name_address)),
              "root");
}

TEST(ReflectionSystemTests, InheritedDescriptorConstructsGrandChild)
{
    ensure_reflection_ready();

    TestGrandChild grand_child;
    const reflect::ClassInfo* cls = TestGrandChild::static_class();
    ASSERT_NE(cls, nullptr);

    const reflect::PropertyDescriptor* descriptor =
      cls->find_property("root_name");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->construct, nullptr);

    auto property = descriptor->construct(
      &grand_child,
      descriptor->name,
      descriptor->label,
      descriptor->flags,
      descriptor->constraint);
    ASSERT_NE(property, nullptr);
    ASSERT_TRUE(property->is_type<reflect::StringProperty>());
}

TEST(ReflectionSystemTests, ErasedConstructAdjustsMultiInheritance)
{
    ensure_reflection_ready();

    OffsetChild obj;
    const reflect::ClassInfo* cls = OffsetChild::static_class();
    ASSERT_NE(cls, nullptr);

    const reflect::PropertyDescriptor* descriptor =
      cls->find_property("local_flag");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->construct, nullptr);

    // The erased pointer is expected to be a Root-subobject pointer.
    void* erased_obj = static_cast<TestRoot*>(&obj);
    auto property = descriptor->construct(
      erased_obj,
      descriptor->name,
      descriptor->label,
      descriptor->flags,
      descriptor->constraint);
    ASSERT_NE(property, nullptr);
    ASSERT_TRUE(property->is_type<reflect::BoolProperty>());

    auto& bool_property = property->as<reflect::BoolProperty>();
    auto bool_property_address = reinterpret_cast<std::uintptr_t>(&obj)
                                 + property->get_offset();
    EXPECT_FALSE(bool_property.get_value(
      reinterpret_cast<const void*>(bool_property_address)));
    EXPECT_TRUE(bool_property.set_value(
      reinterpret_cast<void*>(bool_property_address), true));
    EXPECT_TRUE(obj.local_flag);
    EXPECT_EQ(obj.sentinel, 0xA5A5A5A5u);
}

TEST(ReflectionSystemTests, PreservesPropertyRegistrationOrder)
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

TEST(ReflectionSystemTests, SupportsExplicitEmptyProps)
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

TEST(ReflectionSystemTests, RootCanOmitProps)
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

TEST(ReflectionSystemTests, EmptyChildFindsInheritedProps)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = EmptyRegisteredChild::static_class();

    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->first_property, nullptr);
    EXPECT_NE(cls->find_property("root_value"), nullptr);
    EXPECT_NE(cls->find_property("root_name"), nullptr);
    EXPECT_EQ(cls->find_property("missing_property"), nullptr);
}

TEST(ReflectionSystemTests, DescriptorConstructNullThrows)
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
        descriptor->flags,
        descriptor->constraint),
      reflect::InstanceError);
}

TEST(ReflectionSystemTests, DescriptorConstructWrongTypeThrows)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* child_cls = TestChild::static_class();
    ASSERT_NE(child_cls, nullptr);
    ASSERT_NE(child_cls->first_property, nullptr);

    const reflect::PropertyDescriptor* descriptor = child_cls->first_property.get();
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->construct, nullptr);

    TestRoot wrong_object;
    EXPECT_THROW(
      descriptor->construct(
        &wrong_object,
        descriptor->name,
        descriptor->label,
        descriptor->flags,
        descriptor->constraint),
      reflect::InstanceError);
}

TEST(ReflectionSystemTests, DescriptorAndPropertyCarryConstraint)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = ConstrainedChild::static_class();
    ASSERT_NE(cls, nullptr);
    const reflect::PropertyDescriptor* descriptor =
      cls->find_property("constrained_value");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->constraint, nullptr);

    const auto* descriptor_constraint_base = descriptor->constraint.get();
    ASSERT_NE(descriptor_constraint_base, nullptr);
    ASSERT_EQ(
      descriptor_constraint_base->get_type_tag(),
      reflect::detail::type_tag<ConstrainedChild::CustomConstraint>());
    const auto* descriptor_constraint =
      static_cast<const ConstrainedChild::CustomConstraint*>(
        descriptor_constraint_base);
    ASSERT_NE(descriptor_constraint, nullptr);
    EXPECT_EQ(descriptor_constraint->marker, 77);

    ConstrainedChild instance;
    auto property = descriptor->construct(
      &instance,
      descriptor->name,
      descriptor->label,
      descriptor->flags,
      descriptor->constraint);
    ASSERT_NE(property, nullptr);

    const auto* property_constraint =
      property->try_get_constraint<ConstrainedChild::CustomConstraint>();
    ASSERT_NE(property_constraint, nullptr);
    EXPECT_EQ(property_constraint->marker, 77);
}

TEST(ReflectionSystemTests, ConstructedIntPropertyClampsToRange)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = RangeConstrainedChild::static_class();
    ASSERT_NE(cls, nullptr);
    const reflect::PropertyDescriptor* descriptor =
      cls->find_property("constrained_teeth");
    ASSERT_NE(descriptor, nullptr);

    const auto* descriptor_constraint_base = descriptor->constraint.get();
    ASSERT_NE(descriptor_constraint_base, nullptr);
    ASSERT_EQ(
      descriptor_constraint_base->get_type_tag(),
      reflect::detail::type_tag<reflect::RangeConstraint<int>>());
    const auto* descriptor_range =
      static_cast<const reflect::RangeConstraint<int>*>(
        descriptor_constraint_base);
    ASSERT_TRUE(descriptor_range->min.has_value());
    ASSERT_TRUE(descriptor_range->max.has_value());
    ASSERT_TRUE(descriptor_range->clamp);
    EXPECT_EQ(*descriptor_range->min, 5);
    EXPECT_EQ(*descriptor_range->max, 50);

    RangeConstrainedChild instance;
    auto property = descriptor->construct(
      &instance,
      descriptor->name,
      descriptor->label,
      descriptor->flags,
      descriptor->constraint);
    ASSERT_NE(property, nullptr);
    auto property_address = reinterpret_cast<void*>(
      reinterpret_cast<std::uintptr_t>(&instance) + property->get_offset());

    auto* int_property = property->try_as<reflect::IntProperty>();
    ASSERT_NE(int_property, nullptr);

    EXPECT_TRUE(int_property->set_value(property_address, 1));
    EXPECT_EQ(instance.constrained_teeth, 5);

    EXPECT_TRUE(int_property->set_value(property_address, 100));
    EXPECT_EQ(instance.constrained_teeth, 50);
}

TEST(ReflectionSystemTests, DescriptorCarriesTypedDefaultMetadata)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = DefaultedChild::static_class();
    ASSERT_NE(cls, nullptr);
    const reflect::PropertyDescriptor* descriptor =
      cls->find_property("defaulted_value");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->get_default_value(), nullptr);

    const auto* typed_default = descriptor->try_get_default<int>();
    ASSERT_NE(typed_default, nullptr);
    EXPECT_EQ(typed_default->value, 42);
}

TEST(ReflectionSystemTests, DescriptorDefaultLookupRequiresExactType)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = DefaultedChild::static_class();
    ASSERT_NE(cls, nullptr);
    const reflect::PropertyDescriptor* descriptor =
      cls->find_property("defaulted_value");
    ASSERT_NE(descriptor, nullptr);

    EXPECT_EQ(descriptor->try_get_default<float>(), nullptr);
}

TEST(ReflectionSystemTests, DescriptorCanCarryConstraintAndDefaultMetadata)
{
    ensure_reflection_ready();

    const reflect::ClassInfo* cls = DefaultedChild::static_class();
    ASSERT_NE(cls, nullptr);
    const reflect::PropertyDescriptor* descriptor =
      cls->find_property("ranged_defaulted_value");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_NE(descriptor->constraint, nullptr);
    ASSERT_NE(descriptor->get_default_value(), nullptr);

    const auto* range = static_cast<const reflect::RangeConstraint<float>*>(
      descriptor->constraint.get());
    ASSERT_NE(range, nullptr);
    ASSERT_TRUE(range->min.has_value());
    ASSERT_TRUE(range->max.has_value());
    EXPECT_FLOAT_EQ(*range->min, 0.0f);
    EXPECT_FLOAT_EQ(*range->max, 1.0f);

    const auto* typed_default = descriptor->try_get_default<float>();
    ASSERT_NE(typed_default, nullptr);
    EXPECT_FLOAT_EQ(typed_default->value, 0.5f);
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

TEST(ReflectionSystemTests, FindPropertyPrefersDerived)
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

TEST(ReflectionSystemTests, ClearThenReregisterWorks)
{
    ensure_reflection_ready();

    reflect::ClassInfo class_storage{};
    reflect::detail::PendingClassRegistration reg{
      .module_name = "RuntimeClear",
      .name = "TempClass",
      .size = sizeof(int),
      .storage = &class_storage,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootClear>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassNode node{.reg = &reg, .next = nullptr};
    reflect::detail::AutoClassRegistrar registrar{&node};
    EXPECT_NO_THROW(reflect::ReflectionSystem::process_pending_registrations());
    ASSERT_NE(
      reflect::ReflectionSystem::find_class(
        "RuntimeClear.TempClass",
        reflect::detail::root_type_tag<RuntimeRootClear>()),
      nullptr);

    reflect::ReflectionSystem::clear();
    EXPECT_EQ(
      reflect::ReflectionSystem::find_class(
        "RuntimeClear.TempClass",
        reflect::detail::root_type_tag<RuntimeRootClear>()),
      nullptr);

    reflect::ClassInfo class_storage2{};
    reflect::detail::PendingClassRegistration reg2{
      .module_name = "RuntimeClear",
      .name = "TempClass",
      .size = sizeof(int),
      .storage = &class_storage2,
      .resolve_super = nullptr,
      .root_tag = reflect::detail::root_type_tag<RuntimeRootClear>(),
      .factory = nullptr,
      .destroy = nullptr,
      .register_properties = nullptr};
    reflect::detail::PendingClassNode node2{.reg = &reg2, .next = nullptr};
    reflect::detail::AutoClassRegistrar registrar2{&node2};
    EXPECT_NO_THROW(reflect::ReflectionSystem::process_pending_registrations());
    EXPECT_NE(
      reflect::ReflectionSystem::find_class(
        "RuntimeClear.TempClass",
        reflect::detail::root_type_tag<RuntimeRootClear>()),
      nullptr);

    reflect::ReflectionSystem::unregister_module("RuntimeClear");
}

TEST(ReflectionSystemTests, ConstructFromName)
{
    ensure_reflection_ready();

    // Construct instance from name
    reflect::unique_ptr<TestRoot> obj =
      reflect::construct<TestRoot>("Test.TestChild");
    ASSERT_NE(obj, nullptr);
    EXPECT_TRUE(obj->is_a(TestChild::static_class()));
    EXPECT_TRUE(obj->is_a(TestRoot::static_class()));
}

TEST(ReflectionSystemTests, ConstructFromClass)
{
    ensure_reflection_ready();

    // Construct instance from class
    reflect::unique_ptr<TestRoot> obj =
      reflect::construct<TestRoot>(TestChild::static_class());
    ASSERT_NE(obj, nullptr);
    EXPECT_TRUE(obj->is_a(TestChild::static_class()));
    EXPECT_TRUE(obj->is_a(TestRoot::static_class()));
}

TEST(ReflectionSystemTests, ConstructAndInit)
{
    ensure_reflection_ready();

    std::size_t destructor_calls{0};

    {
        // Construct instance from class with arguments
        reflect::unique_ptr<TestChild> obj =
          reflect::construct_and_init<TestRoot, TestChild>(&destructor_calls);
        ASSERT_NE(obj, nullptr);
        EXPECT_TRUE(obj->is_a(TestChild::static_class()));
        EXPECT_TRUE(obj->is_a(TestRoot::static_class()));
    }

    EXPECT_EQ(destructor_calls, 2);
}

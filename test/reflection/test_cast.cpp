#include <gtest/gtest.h>

#include "reflection/cast.h"
#include "reflection/class_registry.h"
#include "reflection/except.h"

class CastTestRoot : public reflect::ReflectRoot<CastTestRoot>
{
public:
    static void register_properties([[maybe_unused]] reflect::ClassInfo& class_info)
    {
    }

    virtual ~CastTestRoot() = default;
    int root_value{1};
};

DECLARE_REFLECTION(CastTest, CastTestRoot);
DEFINE_REFLECTION(CastTestRoot);

class CastTestChild : public reflect::Reflected<CastTestChild, CastTestRoot>
{
public:
    static void register_properties([[maybe_unused]] reflect::ClassInfo& class_info)
    {
    }

    int child_value{2};
};

DECLARE_REFLECTION(CastTest, CastTestChild);
DEFINE_REFLECTION(CastTestChild);

class CastTestGrandChild : public reflect::Reflected<CastTestGrandChild, CastTestChild>
{
public:
    static void register_properties([[maybe_unused]] reflect::ClassInfo& class_info)
    {
    }

    int grandchild_value{3};
};

DECLARE_REFLECTION(CastTest, CastTestGrandChild);
DEFINE_REFLECTION(CastTestGrandChild);

class CastTestSibling : public reflect::Reflected<CastTestSibling, CastTestRoot>
{
public:
    static void register_properties([[maybe_unused]] reflect::ClassInfo& class_info)
    {
    }
};

DECLARE_REFLECTION(CastTest, CastTestSibling);
DEFINE_REFLECTION(CastTestSibling);

void ensure_cast_reflection_ready()
{
    reflect::ReflectionSystem::allow_auto_registration(false);
    reflect::ReflectionSystem::process_pending_registrations();
}

TEST(CastTests, TryCastReturnsNullptrForNullPointer)
{
    ensure_cast_reflection_ready();
    CastTestRoot* root_ptr = nullptr;
    EXPECT_EQ((reflect::try_cast<CastTestChild>(root_ptr)), nullptr);
}

TEST(CastTests, TryCastSucceedsForDerivedType)
{
    ensure_cast_reflection_ready();
    CastTestGrandChild grandchild;
    CastTestRoot* root_ptr = &grandchild;

    auto* child_ptr = reflect::try_cast<CastTestChild>(root_ptr);
    ASSERT_NE(child_ptr, nullptr);
    EXPECT_EQ(child_ptr->child_value, 2);

    auto* grandchild_ptr = reflect::try_cast<CastTestGrandChild>(root_ptr);
    ASSERT_NE(grandchild_ptr, nullptr);
    EXPECT_EQ(grandchild_ptr->grandchild_value, 3);
}

TEST(CastTests, TryCastReturnsNullptrForTypeMismatch)
{
    ensure_cast_reflection_ready();
    CastTestSibling sibling;
    CastTestRoot* root_ptr = &sibling;
    EXPECT_EQ((reflect::try_cast<CastTestChild>(root_ptr)), nullptr);
}

TEST(CastTests, TryCastSupportsConstPointers)
{
    ensure_cast_reflection_ready();
    const CastTestChild child;
    const CastTestRoot* root_ptr = &child;
    auto* casted_ptr = reflect::try_cast<CastTestChild>(root_ptr);
    ASSERT_NE(casted_ptr, nullptr);
    EXPECT_EQ(casted_ptr->child_value, 2);
}

TEST(CastTests, CastReturnsReferenceOnSuccess)
{
    ensure_cast_reflection_ready();
    CastTestGrandChild grandchild;
    CastTestRoot& root_ref = grandchild;
    auto& child_ref = reflect::cast<CastTestChild>(root_ref);
    EXPECT_EQ(child_ref.child_value, 2);
}

TEST(CastTests, CastThrowsInstanceErrorOnMismatch)
{
    ensure_cast_reflection_ready();
    CastTestSibling sibling;
    CastTestRoot& root_ref = sibling;
    EXPECT_THROW(
      {
          [[maybe_unused]] auto& ref = reflect::cast<CastTestChild>(root_ref);
      },
      reflect::InstanceError);
}

TEST(CastTests, CastSupportsConstReferences)
{
    ensure_cast_reflection_ready();
    const CastTestGrandChild grandchild;
    const CastTestRoot& root_ref = grandchild;
    const auto& child_ref = reflect::cast<CastTestChild>(root_ref);
    EXPECT_EQ(child_ref.child_value, 2);
}

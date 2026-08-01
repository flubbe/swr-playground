#include <gtest/gtest.h>

#include "reflection/class_info.h"

TEST(ClassInfoTests, ReturnsAssignedSuperClass)
{
    reflect::ClassInfo super{};
    super.qualified_name = "Test.Super";

    reflect::ClassInfo child{};
    child.qualified_name = "Test.Child";
    child.super = &super;

    EXPECT_EQ(child.get_super(), &super);
    EXPECT_EQ(child.get_super(), &super);
}

TEST(ClassInfoTests, ReturnsNullWhenNoSuperClassAssigned)
{
    reflect::ClassInfo child{};
    child.qualified_name = "Test.ChildWithoutSuper";
    EXPECT_EQ(child.get_super(), nullptr);
}

TEST(ClassInfoTests, IsAReportsSelfAndSuperHierarchy)
{
    reflect::ClassInfo root{};
    root.qualified_name = "Test.Root";

    reflect::ClassInfo mid{};
    mid.qualified_name = "Test.Mid";
    mid.super = &root;

    reflect::ClassInfo leaf{};
    leaf.qualified_name = "Test.Leaf";
    leaf.super = &mid;

    EXPECT_TRUE(leaf.is_a(&leaf));
    EXPECT_TRUE(leaf.is_a(&mid));
    EXPECT_TRUE(leaf.is_a(&root));
    EXPECT_FALSE(mid.is_a(&leaf));
}

TEST(ClassInfoTests, FindPropertySearchesCurrentClassBeforeSuper)
{
    reflect::ClassInfo root{};
    root.qualified_name = "Test.Root";
    root.first_property = swr::make_unique<reflect::PropertyDescriptor>(
      "shared_name",
      "Root Label",
      reflect::PropertyFlags::None,
      nullptr,
      nullptr);

    reflect::ClassInfo leaf{};
    leaf.qualified_name = "Test.Leaf";
    leaf.super = &root;
    leaf.first_property = swr::make_unique<reflect::PropertyDescriptor>(
      "shared_name",
      "Leaf Label",
      reflect::PropertyFlags::ReadOnly,
      nullptr,
      nullptr);

    const reflect::PropertyDescriptor* found = leaf.find_property("shared_name");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->label, "Leaf Label");
}

TEST(ClassInfoTests, FindPropertyFallsBackToSuperHierarchy)
{
    reflect::ClassInfo root{};
    root.qualified_name = "Test.Root";
    root.first_property = swr::make_unique<reflect::PropertyDescriptor>(
      "root_only",
      "Root Property",
      reflect::PropertyFlags::None,
      nullptr,
      nullptr);

    reflect::ClassInfo mid{};
    mid.qualified_name = "Test.Mid";
    mid.super = &root;

    reflect::ClassInfo leaf{};
    leaf.qualified_name = "Test.Leaf";
    leaf.super = &mid;

    const reflect::ClassInfo& leaf_view = leaf;
    const reflect::PropertyDescriptor* found = leaf_view.find_property("root_only");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "root_only");
    EXPECT_EQ(found->label, "Root Property");
}

TEST(ClassInfoTests, FindPropertyReturnsNullWhenMissing)
{
    reflect::ClassInfo root{};
    root.qualified_name = "Test.Root";

    reflect::ClassInfo leaf{};
    leaf.qualified_name = "Test.Leaf";
    leaf.super = &root;

    EXPECT_EQ(leaf.find_property("missing"), nullptr);
}

TEST(ClassInfoTests, IsAReturnsFalseForNullClassPointer)
{
    reflect::ClassInfo cls{};
    cls.qualified_name = "Test.Class";
    EXPECT_FALSE(cls.is_a(nullptr));
}

TEST(ClassInfoTests, NonConstFindPropertyDoesNotTraverseSuperClasses)
{
    reflect::ClassInfo root{};
    root.qualified_name = "Test.Root";
    root.first_property = swr::make_unique<reflect::PropertyDescriptor>(
      "root_only",
      "Root Property",
      reflect::PropertyFlags::None,
      nullptr,
      nullptr);

    reflect::ClassInfo leaf{};
    leaf.qualified_name = "Test.Leaf";
    leaf.super = &root;

    EXPECT_EQ(leaf.find_property("root_only"), nullptr);

    const reflect::ClassInfo& leaf_view = leaf;
    EXPECT_NE(leaf_view.find_property("root_only"), nullptr);
}

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

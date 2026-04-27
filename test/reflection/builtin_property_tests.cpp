#include <stdexcept>

#include <gtest/gtest.h>

#include "reflection/builtin_properties.h"

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

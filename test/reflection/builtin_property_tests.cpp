#include <stdexcept>
#include <cstdint>

#include <gtest/gtest.h>

#include "reflection/builtin_properties.h"

TEST(BuiltinPropertyTests, ConstructorsRejectNullValuePointers)
{
    EXPECT_THROW(
      reflect::IntProperty("int_value", "Int Value", nullptr, 0),
      std::invalid_argument);
    EXPECT_THROW(
      reflect::UIntProperty("uint_value", "UInt Value", nullptr, 0),
      std::invalid_argument);
    EXPECT_THROW(
      reflect::FloatProperty("float_value", "Float Value", nullptr, 0),
      std::invalid_argument);
    EXPECT_THROW(
      reflect::BoolProperty("bool_value", "Bool Value", nullptr, 0),
      std::invalid_argument);
    EXPECT_THROW(
      reflect::StringProperty("string_value", "String Value", nullptr, 0),
      std::invalid_argument);
}

TEST(BuiltinPropertyTests, ReadOnlyIntPropertyPreventsWrites)
{
    int value = 7;
    reflect::IntProperty property(
      "value",
      "Value",
      &value,
      0,
      reflect::PropertyFlags::ReadOnly);

    EXPECT_FALSE(property.set_value(11));
    EXPECT_EQ(value, 7);
}

TEST(BuiltinPropertyTests, TypeTagCastsWorkForPropertyBase)
{
    int value = 3;
    reflect::IntProperty int_property("value", "Value", &value, 0);
    reflect::Property& property = int_property;

    EXPECT_TRUE(property.is_type<reflect::IntProperty>());
    EXPECT_NE(property.try_as<reflect::IntProperty>(), nullptr);
    EXPECT_EQ(property.try_as<reflect::FloatProperty>(), nullptr);
}

TEST(BuiltinPropertyTests, ExposesSizeAlignmentAndOffsetMetadata)
{
    struct LayoutProbe
    {
        int i{1};
        unsigned int u{2};
        float f{3.0f};
        bool b{true};
        std::string s{"x"};
    };

    LayoutProbe probe{};
    const auto base = reinterpret_cast<std::uintptr_t>(&probe);

    std::unique_ptr<reflect::Property> int_property =
      reflect::PropertyFactory<int>::construct(
        "i",
        "I",
        probe.i,
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.i) - base),
        reflect::PropertyFlags::None);
    std::unique_ptr<reflect::Property> uint_property =
      reflect::PropertyFactory<unsigned int>::construct(
        "u",
        "U",
        probe.u,
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.u) - base),
        reflect::PropertyFlags::None);
    std::unique_ptr<reflect::Property> float_property =
      reflect::PropertyFactory<float>::construct(
        "f",
        "F",
        probe.f,
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.f) - base),
        reflect::PropertyFlags::None);
    std::unique_ptr<reflect::Property> bool_property =
      reflect::PropertyFactory<bool>::construct(
        "b",
        "B",
        probe.b,
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.b) - base),
        reflect::PropertyFlags::None);
    std::unique_ptr<reflect::Property> string_property =
      reflect::PropertyFactory<std::string>::construct(
        "s",
        "S",
        probe.s,
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.s) - base),
        reflect::PropertyFlags::None);

    ASSERT_NE(int_property, nullptr);
    ASSERT_NE(uint_property, nullptr);
    ASSERT_NE(float_property, nullptr);
    ASSERT_NE(bool_property, nullptr);
    ASSERT_NE(string_property, nullptr);

    EXPECT_EQ(int_property->get_size(), sizeof(int));
    EXPECT_EQ(int_property->get_alignment(), alignof(int));
    EXPECT_EQ(
      int_property->get_offset(),
      static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.i) - base));

    EXPECT_EQ(uint_property->get_size(), sizeof(unsigned int));
    EXPECT_EQ(uint_property->get_alignment(), alignof(unsigned int));
    EXPECT_EQ(
      uint_property->get_offset(),
      static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.u) - base));

    EXPECT_EQ(float_property->get_size(), sizeof(float));
    EXPECT_EQ(float_property->get_alignment(), alignof(float));
    EXPECT_EQ(
      float_property->get_offset(),
      static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.f) - base));

    EXPECT_EQ(bool_property->get_size(), sizeof(bool));
    EXPECT_EQ(bool_property->get_alignment(), alignof(bool));
    EXPECT_EQ(
      bool_property->get_offset(),
      static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.b) - base));

    EXPECT_EQ(string_property->get_size(), sizeof(std::string));
    EXPECT_EQ(string_property->get_alignment(), alignof(std::string));
    EXPECT_EQ(
      string_property->get_offset(),
      static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.s) - base));
}

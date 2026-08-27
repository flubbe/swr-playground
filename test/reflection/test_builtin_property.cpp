#include <stdexcept>
#include <cstdint>

#include <gtest/gtest.h>

#include "containers/memory.h"
#include "reflection/builtin_properties.h"

namespace
{

struct CustomIntConstraint : reflect::PropertyConstraint
{
    int marker{0};

    const void* get_type_tag() const noexcept override
    {
        return reflect::detail::type_tag<CustomIntConstraint>();
    }
};

}    // namespace

TEST(BuiltinPropertyTests, TypeTagCastsWorkForPropertyBase)
{
    reflect::IntProperty int_property("value", "Value", 0, 0);
    reflect::Property& property = int_property;

    EXPECT_TRUE(property.is_type<reflect::IntProperty>());
    EXPECT_NE(property.try_as<reflect::IntProperty>(), nullptr);
    EXPECT_EQ(property.try_as<reflect::FloatProperty>(), nullptr);
}

TEST(BuiltinPropertyTests, IntPropertyClampsWhenRangeConstraintRequestsIt)
{
    reflect::RangeConstraint<int> constraint{};
    constraint.min = 0;
    constraint.max = 10;
    constraint.step = 1;
    constraint.clamp = true;
    reflect::IntProperty property(
      "value",
      "Value",
      0,
      0,
      reflect::PropertyFlags::None,
      1,
      std::make_shared<reflect::RangeConstraint<int>>(constraint));

    int value = 5;
    EXPECT_TRUE(property.set_value(&value, 42));
    EXPECT_EQ(value, 10);
    EXPECT_TRUE(property.set_value(&value, -7));
    EXPECT_EQ(value, 0);
}

TEST(BuiltinPropertyTests, FloatPropertyExposesRangeConstraintMetadata)
{
    reflect::RangeConstraint<float> constraint{};
    constraint.min = 0.0f;
    constraint.max = 1.0f;
    constraint.step = 0.05f;
    constraint.clamp = true;
    reflect::FloatProperty property(
      "value",
      "Value",
      0,
      0,
      reflect::PropertyFlags::None,
      0.01f,
      "%.3f",
      std::make_shared<reflect::RangeConstraint<float>>(constraint));

    const auto* range = property.try_get_range_constraint<float>();
    ASSERT_NE(range, nullptr);
    ASSERT_TRUE(range->min.has_value());
    ASSERT_TRUE(range->max.has_value());
    ASSERT_TRUE(range->step.has_value());
    EXPECT_FLOAT_EQ(*range->min, 0.0f);
    EXPECT_FLOAT_EQ(*range->max, 1.0f);
    EXPECT_FLOAT_EQ(*range->step, 0.05f);
    EXPECT_TRUE(range->clamp);
}

TEST(BuiltinPropertyTests, ConstraintTypeLookupReturnsExactTypeOnly)
{
    auto constraint = std::make_shared<CustomIntConstraint>();
    constraint->marker = 123;

    reflect::IntProperty property(
      "value",
      "Value",
      0,
      0,
      reflect::PropertyFlags::None,
      1.0f,
      constraint);

    const auto* custom = property.try_get_constraint<CustomIntConstraint>();
    ASSERT_NE(custom, nullptr);
    EXPECT_EQ(custom->marker, 123);

    const auto* wrong = property.try_get_constraint<reflect::RangeConstraint<int>>();
    EXPECT_EQ(wrong, nullptr);
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

    swr::unique_ptr<reflect::Property> int_property =
      reflect::PropertyFactory<int>::construct(
        "i",
        "I",
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.i) - base),
        0,
        reflect::PropertyFlags::None,
        {});
    swr::unique_ptr<reflect::Property> uint_property =
      reflect::PropertyFactory<unsigned int>::construct(
        "u",
        "U",
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.u) - base),
        0,
        reflect::PropertyFlags::None,
        {});
    swr::unique_ptr<reflect::Property> float_property =
      reflect::PropertyFactory<float>::construct(
        "f",
        "F",
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.f) - base),
        0,
        reflect::PropertyFlags::None,
        {});
    swr::unique_ptr<reflect::Property> bool_property =
      reflect::PropertyFactory<bool>::construct(
        "b",
        "B",
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.b) - base),
        0,
        reflect::PropertyFlags::None,
        {});
    swr::unique_ptr<reflect::Property> string_property =
      reflect::PropertyFactory<std::string>::construct(
        "s",
        "S",
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(&probe.s) - base),
        0,
        reflect::PropertyFlags::None,
        {});

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

/**
 * Software Rasterizer Playground.
 *
 * Built-in reflected properties.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "builtin_properties.h"

namespace
{

const std::string& empty_string()
{
    static const std::string k_empty{};
    return k_empty;
}

}    // namespace

namespace reflect
{

IntProperty::IntProperty(
  std::string name,
  std::string label,
  int* value,
  PropertyFlags flags,
  float speed)
: Property{std::move(name), std::move(label), flags}
, value{value}
, speed{speed}
{
}

bool IntProperty::has_value() const noexcept
{
    return value != nullptr;
}

int IntProperty::get_value() const noexcept
{
    return value != nullptr ? *value : 0;
}

bool IntProperty::set_value(int in_value) noexcept
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    *value = in_value;
    return true;
}

float IntProperty::get_speed() const noexcept
{
    return speed;
}

const void* IntProperty::get_type_tag() const noexcept
{
    return detail::property_type_tag<IntProperty>();
}

void IntProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

UIntProperty::UIntProperty(
  std::string name,
  std::string label,
  unsigned int* value,
  PropertyFlags flags,
  float speed)
: Property{std::move(name), std::move(label), flags}
, value{value}
, speed{speed}
{
}

bool UIntProperty::has_value() const noexcept
{
    return value != nullptr;
}

unsigned int UIntProperty::get_value() const noexcept
{
    return value != nullptr ? *value : 0;
}

bool UIntProperty::set_value(unsigned int in_value) noexcept
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    *value = in_value;
    return true;
}

float UIntProperty::get_speed() const noexcept
{
    return speed;
}

const void* UIntProperty::get_type_tag() const noexcept
{
    return detail::property_type_tag<UIntProperty>();
}

void UIntProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

FloatProperty::FloatProperty(
  std::string name,
  std::string label,
  float* value,
  PropertyFlags flags,
  float speed,
  const char* format)
: Property{std::move(name), std::move(label), flags}
, value{value}
, speed{speed}
, format{format}
{
}

bool FloatProperty::has_value() const noexcept
{
    return value != nullptr;
}

float FloatProperty::get_value() const noexcept
{
    return value != nullptr ? *value : 0.0f;
}

bool FloatProperty::set_value(float in_value) noexcept
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    *value = in_value;
    return true;
}

float FloatProperty::get_speed() const noexcept
{
    return speed;
}

const char* FloatProperty::get_format() const noexcept
{
    return format;
}

const void* FloatProperty::get_type_tag() const noexcept
{
    return detail::property_type_tag<FloatProperty>();
}

void FloatProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

BoolProperty::BoolProperty(
  std::string name,
  std::string label,
  bool* value,
  PropertyFlags flags)
: Property{std::move(name), std::move(label), flags}
, value{value}
{
}

bool BoolProperty::has_value() const noexcept
{
    return value != nullptr;
}

bool BoolProperty::get_value() const noexcept
{
    return value != nullptr ? *value : false;
}

bool BoolProperty::set_value(bool in_value) noexcept
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    *value = in_value;
    return true;
}

const void* BoolProperty::get_type_tag() const noexcept
{
    return detail::property_type_tag<BoolProperty>();
}

void BoolProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

StringProperty::StringProperty(
  std::string name,
  std::string label,
  std::string* value,
  PropertyFlags flags,
  std::size_t max_length)
: Property{std::move(name), std::move(label), flags}
, value{value}
, max_length{max_length}
{
}

bool StringProperty::has_value() const noexcept
{
    return value != nullptr;
}

const std::string& StringProperty::get_value() const noexcept
{
    return value != nullptr ? *value : empty_string();
}

bool StringProperty::set_value(std::string_view in_value)
{
    if(value == nullptr || is_read_only())
    {
        return false;
    }

    const std::size_t count = std::min(in_value.size(), max_length);
    value->assign(in_value.data(), count);
    return true;
}

std::size_t StringProperty::get_max_length() const noexcept
{
    return max_length;
}

const void* StringProperty::get_type_tag() const noexcept
{
    return detail::property_type_tag<StringProperty>();
}

void StringProperty::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

}    // namespace reflect

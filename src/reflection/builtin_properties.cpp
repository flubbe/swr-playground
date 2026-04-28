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

#include <stdexcept>

namespace reflect
{

IntProperty::IntProperty(
  std::string name,
  std::string label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  float speed)
: Property{
    std::move(name),
    std::move(label),
    sizeof(Type),
    offset,
    alignof(Type),
    flags}
, value{value}
, speed{speed}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"IntProperty requires non-null value pointer"};
    }
}

IntProperty::Type IntProperty::get_value() const noexcept
{
    return *value;
}

bool IntProperty::set_value(Type in_value) noexcept
{
    if(is_read_only())
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

UIntProperty::UIntProperty(
  std::string name,
  std::string label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  float speed)
: Property{
    std::move(name),
    std::move(label),
    sizeof(Type),
    offset,
    alignof(Type),
    flags}
, value{value}
, speed{speed}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"UIntProperty requires non-null value pointer"};
    }
}

UIntProperty::Type UIntProperty::get_value() const noexcept
{
    return *value;
}

bool UIntProperty::set_value(Type in_value) noexcept
{
    if(is_read_only())
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

FloatProperty::FloatProperty(
  std::string name,
  std::string label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  float speed,
  const char* format)
: Property{
    std::move(name),
    std::move(label),
    sizeof(Type),
    offset,
    alignof(Type),
    flags}
, value{value}
, speed{speed}
, format{format}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"FloatProperty requires non-null value pointer"};
    }
}

FloatProperty::Type FloatProperty::get_value() const noexcept
{
    return *value;
}

bool FloatProperty::set_value(Type in_value) noexcept
{
    if(is_read_only())
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

BoolProperty::BoolProperty(
  std::string name,
  std::string label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags)
: Property{
    std::move(name),
    std::move(label),
    sizeof(Type),
    offset,
    alignof(Type),
    flags}
, value{value}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"BoolProperty requires non-null value pointer"};
    }
}

BoolProperty::Type BoolProperty::get_value() const noexcept
{
    return *value;
}

bool BoolProperty::set_value(Type in_value) noexcept
{
    if(is_read_only())
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

StringProperty::StringProperty(
  std::string name,
  std::string label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  std::size_t max_length)
: Property{
    std::move(name),
    std::move(label),
    sizeof(Type),
    offset,
    alignof(Type),
    flags}
, value{value}
, max_length{max_length}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"StringProperty requires non-null value pointer"};
    }
}

const StringProperty::Type& StringProperty::get_value() const noexcept
{
    return *value;
}

bool StringProperty::set_value(std::string_view in_value)
{
    if(is_read_only())
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

}    // namespace reflect

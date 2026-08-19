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

#include <algorithm>
#include <stdexcept>

namespace reflect
{

/*
 * IntProperty.
 */

IntProperty::IntProperty(
  std::string_view name,
  std::string_view label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  float speed,
  swr::shared_ptr<const PropertyConstraint> constraint)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    flags,
    constraint}
, value{value}
, speed{speed}
, range_constraint{range_constraint_from_metadata<Type>()}
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
    if(range_constraint.has_value())
    {
        if(range_constraint->min.has_value()
           && in_value < *range_constraint->min)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            in_value = *range_constraint->min;
        }
        if(range_constraint->max.has_value()
           && in_value > *range_constraint->max)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            in_value = *range_constraint->max;
        }
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
    return detail::type_tag<IntProperty>();
}

/*
 * UIntProperty.
 */

UIntProperty::UIntProperty(
  std::string_view name,
  std::string_view label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  float speed,
  swr::shared_ptr<const PropertyConstraint> constraint)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    flags,
    constraint}
, value{value}
, speed{speed}
, range_constraint{range_constraint_from_metadata<Type>()}
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
    if(range_constraint.has_value())
    {
        if(range_constraint->min.has_value() && in_value < *range_constraint->min)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            in_value = *range_constraint->min;
        }
        if(range_constraint->max.has_value() && in_value > *range_constraint->max)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            in_value = *range_constraint->max;
        }
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
    return detail::type_tag<UIntProperty>();
}

/*
 * FloatProperty.
 */

FloatProperty::FloatProperty(
  std::string_view name,
  std::string_view label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  float speed,
  const char* format,
  swr::shared_ptr<const PropertyConstraint> constraint)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    flags,
    constraint}
, value{value}
, speed{speed}
, format{format}
, range_constraint{range_constraint_from_metadata<Type>()}
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
    if(range_constraint.has_value())
    {
        if(range_constraint->min.has_value() && in_value < *range_constraint->min)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            in_value = *range_constraint->min;
        }
        if(range_constraint->max.has_value() && in_value > *range_constraint->max)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            in_value = *range_constraint->max;
        }
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
    return detail::type_tag<FloatProperty>();
}

/*
 * BoolProperty.
 */

BoolProperty::BoolProperty(
  std::string_view name,
  std::string_view label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  swr::shared_ptr<const PropertyConstraint> constraint)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    flags,
    std::move(constraint)}
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
    *value = in_value;
    return true;
}

const void* BoolProperty::get_type_tag() const noexcept
{
    return detail::type_tag<BoolProperty>();
}

/*
 * StringProperty.
 */

StringProperty::StringProperty(
  std::string_view name,
  std::string_view label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  std::size_t max_length,
  swr::shared_ptr<const PropertyConstraint> constraint)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    flags,
    std::move(constraint)}
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
    return detail::type_tag<StringProperty>();
}

/*
 * VectorProperty.
 */

const void* VectorProperty::get_type_tag() const noexcept
{
    return detail::type_tag<VectorProperty>();
}

/*
 * PathProperty.
 */

PathProperty::PathProperty(
  std::string_view name,
  std::string_view label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags,
  swr::shared_ptr<const PropertyConstraint> constraint)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    flags,
    std::move(constraint)}
, value{value}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"PathProperty requires non-null value pointer"};
    }
}

const PathProperty::Type& PathProperty::get_value() const noexcept
{
    return *value;
}

bool PathProperty::set_value(const std::filesystem::path& in_value)
{
    value->assign(in_value);
    return true;
}

const void* PathProperty::get_type_tag() const noexcept
{
    return detail::type_tag<PathProperty>();
}

}    // namespace reflect

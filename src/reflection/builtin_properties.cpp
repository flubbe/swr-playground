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
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  Type speed,
  swr::shared_ptr<const PropertyConstraint> constraint)
: TypedProperty{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    element_count,
    flags,
    constraint}
, speed{speed}
, range_constraint{range_constraint_from_metadata<Type>()}
{
}

bool IntProperty::set_value(
  void* storage,
  const Type& value) const
{
    Type clamped_value = value;

    if(range_constraint.has_value())
    {
        if(range_constraint->min.has_value()
           && clamped_value < *range_constraint->min)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            clamped_value = *range_constraint->min;
        }
        if(range_constraint->max.has_value()
           && clamped_value > *range_constraint->max)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            clamped_value = *range_constraint->max;
        }
    }

    *static_cast<Type*>(storage) = clamped_value;
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

swr::unique_ptr<IntProperty> IntProperty::construct(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  Type speed,
  swr::shared_ptr<const PropertyConstraint> constraint)
{
    return swr::make_unique<IntProperty>(
      name,
      label,
      offset,
      element_count,
      flags,
      speed,
      std::move(constraint));
}

/*
 * UIntProperty.
 */

UIntProperty::UIntProperty(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  Type speed,
  swr::shared_ptr<const PropertyConstraint> constraint)
: TypedProperty{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    element_count,
    flags,
    constraint}
, speed{speed}
, range_constraint{range_constraint_from_metadata<Type>()}
{
}

bool UIntProperty::set_value(
  void* storage,
  const Type& value) const
{
    Type clamped_value = value;

    if(range_constraint.has_value())
    {
        if(range_constraint->min.has_value()
           && clamped_value < *range_constraint->min)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            clamped_value = *range_constraint->min;
        }
        if(range_constraint->max.has_value()
           && clamped_value > *range_constraint->max)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            clamped_value = *range_constraint->max;
        }
    }

    *static_cast<Type*>(storage) = clamped_value;
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

swr::unique_ptr<UIntProperty> UIntProperty::construct(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  Type speed,
  swr::shared_ptr<const PropertyConstraint> constraint)
{
    return swr::make_unique<UIntProperty>(
      name,
      label,
      offset,
      element_count,
      flags,
      speed,
      std::move(constraint));
}

/*
 * FloatProperty.
 */

FloatProperty::FloatProperty(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  Type speed,
  const char* format,
  swr::shared_ptr<const PropertyConstraint> constraint)
: TypedProperty{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    element_count,
    flags,
    constraint}
, speed{speed}
, format{format}
, range_constraint{range_constraint_from_metadata<Type>()}
{
}

bool FloatProperty::set_value(
  void* storage,
  const Type& value) const
{
    Type clamped_value = value;

    if(range_constraint.has_value())
    {
        if(range_constraint->min.has_value()
           && clamped_value < *range_constraint->min)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            clamped_value = *range_constraint->min;
        }
        if(range_constraint->max.has_value()
           && clamped_value > *range_constraint->max)
        {
            if(!range_constraint->clamp)
            {
                return false;
            }
            clamped_value = *range_constraint->max;
        }
    }

    *static_cast<Type*>(storage) = clamped_value;
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

swr::unique_ptr<FloatProperty> FloatProperty::construct(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  Type speed,
  const char* format,
  swr::shared_ptr<const PropertyConstraint> constraint)
{
    return swr::make_unique<FloatProperty>(
      name,
      label,
      offset,
      element_count,
      flags,
      speed,
      format,
      std::move(constraint));
}

/*
 * BoolProperty.
 */

BoolProperty::BoolProperty(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  swr::shared_ptr<const PropertyConstraint> constraint)
: TypedProperty{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    element_count,
    flags,
    std::move(constraint)}
{
}

const void* BoolProperty::get_type_tag() const noexcept
{
    return detail::type_tag<BoolProperty>();
}

swr::unique_ptr<BoolProperty> BoolProperty::construct(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  swr::shared_ptr<const PropertyConstraint> constraint)
{
    return swr::make_unique<BoolProperty>(
      name,
      label,
      offset,
      element_count,
      flags,
      std::move(constraint));
}

/*
 * StringProperty.
 */

StringProperty::StringProperty(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  std::size_t max_length,
  swr::shared_ptr<const PropertyConstraint> constraint)
: TypedProperty{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    element_count,
    flags,
    std::move(constraint)}
, max_length{max_length}
{
}

bool StringProperty::set_value(
  void* storage,
  const swr::string& value) const
{
    return set_value(
      storage,
      std::string_view{value});
}

bool StringProperty::set_value(
  void* storage,
  std::string_view value) const
{
    const std::size_t count = std::min(value.size(), max_length);
    static_cast<Type*>(storage)->assign(value.data(), count);
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

swr::unique_ptr<StringProperty> StringProperty::construct(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  std::size_t max_length,
  swr::shared_ptr<const PropertyConstraint> constraint)
{
    return swr::make_unique<StringProperty>(
      name,
      label,
      offset,
      element_count,
      flags,
      max_length,
      std::move(constraint));
}

/*
 * VectorProperty.
 */

void VectorProperty::copy_value(
  void* dst,
  const void* src) const
{
    assign_fn(dst, src);
}

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
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  swr::shared_ptr<const PropertyConstraint> constraint)
: TypedProperty{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    element_count,
    flags,
    std::move(constraint)}
{
}

bool PathProperty::set_value(
  void* storage,
  const std::filesystem::path& value) const
{
    static_cast<std::filesystem::path*>(storage)->assign(value);
    return true;
}

const void* PathProperty::get_type_tag() const noexcept
{
    return detail::type_tag<PathProperty>();
}

swr::unique_ptr<PathProperty> PathProperty::construct(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  swr::shared_ptr<const PropertyConstraint> constraint)
{
    return swr::make_unique<PathProperty>(
      name,
      label,
      offset,
      element_count,
      flags,
      std::move(constraint));
}

}    // namespace reflect

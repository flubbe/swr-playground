/**
 * Software Rasterizer Playground.
 *
 * Additional reflected properties.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "properties.h"

#include <stdexcept>

namespace reflect
{

#if SWR_CUSTOM_STRING_TYPE

SwrStringProperty::SwrStringProperty(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags,
  std::size_t max_length,
  swr::shared_ptr<const PropertyConstraint> constraint)
: Property{
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

void SwrStringProperty::copy_value(
  void* dst,
  const void* src) const
{
    *static_cast<Type*>(dst) = *static_cast<const Type*>(src);
}

const SwrStringProperty::Type& SwrStringProperty::get_value(
  const void* storage) const noexcept
{
    return *static_cast<const Type*>(storage);
}

bool SwrStringProperty::set_value(
  void* storage,
  std::string_view in_value) const
{
    const std::size_t count = std::min(in_value.size(), max_length);
    static_cast<Type*>(storage)->assign(in_value.data(), count);
    return true;
}

std::size_t SwrStringProperty::get_max_length() const noexcept
{
    return max_length;
}

const void* SwrStringProperty::get_type_tag() const noexcept
{
    return detail::type_tag<SwrStringProperty>();
}

#endif /* SWR_CUSTOM_STRING_TYPE */

Vec4Property::Vec4Property(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    element_count,
    flags,
    nullptr}
{
}

void Vec4Property::copy_value(
  void* dst,
  const void* src) const
{
    *static_cast<Type*>(dst) = *static_cast<const Type*>(src);
}

const Vec4Property::Type& Vec4Property::get_value(
  const void* storage) const noexcept
{
    return *static_cast<const Type*>(storage);
}

bool Vec4Property::set_value(
  void* storage,
  const Type& value) const noexcept
{
    *static_cast<Type*>(storage) = value;
    return true;
}

const void* Vec4Property::get_type_tag() const noexcept
{
    return detail::type_tag<Vec4Property>();
}

Mat4Property::Mat4Property(
  std::string_view name,
  std::string_view label,
  std::size_t offset,
  std::size_t element_count,
  PropertyFlags flags)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    element_count,
    flags,
    nullptr}
{
}

void Mat4Property::copy_value(
  void* dst,
  const void* src) const
{
    *static_cast<Type*>(dst) = *static_cast<const Type*>(src);
}

const Mat4Property::Type& Mat4Property::get_value(
  const void* storage) const noexcept
{
    return *static_cast<const Type*>(storage);
}

bool Mat4Property::set_value(
  void* storage,
  const Type& value) const noexcept
{
    *static_cast<Type*>(storage) = value;
    return true;
}

const void* Mat4Property::get_type_tag() const noexcept
{
    return detail::type_tag<Mat4Property>();
}

}    // namespace reflect

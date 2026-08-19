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
        throw std::invalid_argument{"SwrStringProperty requires non-null value pointer"};
    }
}

const SwrStringProperty::Type& SwrStringProperty::get_value() const noexcept
{
    return *value;
}

bool SwrStringProperty::set_value(std::string_view in_value)
{
    const std::size_t count = std::min(in_value.size(), max_length);
    value->assign(in_value.data(), count);
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
  Type* value,
  std::size_t offset,
  PropertyFlags flags)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    flags,
    nullptr}
, value{value}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"Vec4Property requires non-null value pointer"};
    }
}

const Vec4Property::Type& Vec4Property::get_value() const noexcept
{
    return *value;
}

bool Vec4Property::set_value(const Type& in_value) noexcept
{
    *value = in_value;
    return true;
}

const void* Vec4Property::get_type_tag() const noexcept
{
    return detail::type_tag<Vec4Property>();
}

Mat4Property::Mat4Property(
  std::string_view name,
  std::string_view label,
  Type* value,
  std::size_t offset,
  PropertyFlags flags)
: Property{
    name,
    label,
    sizeof(Type),
    offset,
    alignof(Type),
    flags,
    nullptr}
, value{value}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"Mat4Property requires non-null value pointer"};
    }
}

const Mat4Property::Type& Mat4Property::get_value() const noexcept
{
    return *value;
}

bool Mat4Property::set_value(const Type& in_value) noexcept
{
    *value = in_value;
    return true;
}

const void* Mat4Property::get_type_tag() const noexcept
{
    return detail::type_tag<Mat4Property>();
}

}    // namespace reflect

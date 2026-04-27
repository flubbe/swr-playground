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

Mat4Property::Mat4Property(
  std::string name,
  std::string label,
  ml::mat4x4* value,
  PropertyFlags flags)
: Property{std::move(name), std::move(label), flags}
, value{value}
{
    if(value == nullptr)
    {
        throw std::invalid_argument{"Mat4Property requires non-null value pointer"};
    }
}

const ml::mat4x4& Mat4Property::get_value() const noexcept
{
    return *value;
}

bool Mat4Property::set_value(const ml::mat4x4& in_value) noexcept
{
    if(is_read_only())
    {
        return false;
    }

    *value = in_value;
    return true;
}

const void* Mat4Property::get_type_tag() const noexcept
{
    return detail::property_type_tag<Mat4Property>();
}

}    // namespace reflect

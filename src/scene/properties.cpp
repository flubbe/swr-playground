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

namespace
{

const ml::mat4x4& identity_mat4()
{
    static const ml::mat4x4 k_identity = ml::mat4x4::identity();
    return k_identity;
}

}    // namespace

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
}

bool Mat4Property::has_value() const noexcept
{
    return value != nullptr;
}

const ml::mat4x4& Mat4Property::get_value() const noexcept
{
    return value != nullptr ? *value : identity_mat4();
}

bool Mat4Property::set_value(const ml::mat4x4& in_value) noexcept
{
    if(value == nullptr || is_read_only())
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

void Mat4Property::accept(PropertyVisitor& visitor)
{
    visitor.visit(*this);
}

}    // namespace reflect

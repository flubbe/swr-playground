/**
 * Software Rasterizer Playground.
 *
 * Additional reflected properties.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "ml/all.h"

#include "reflection/property.h"

namespace reflect
{

/** Reflected 4D vector property. */
class Vec4Property : public Property
{
public:
    using Type = ml::vec4;

private:
    Type* value{nullptr};

public:
    /**
     * Construct a 4D vector property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    Vec4Property(
      std::string name,
      std::string label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    const Type& get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if written, `false` if read-only.
     */
    bool set_value(const Type& in_value) noexcept;
};

/** Reflected 4x4 matrix property. */
class Mat4Property : public Property
{
public:
    using Type = ml::mat4x4;

private:
    Type* value{nullptr};

public:
    /**
     * Construct a 4x4 matrix property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    Mat4Property(
      std::string name,
      std::string label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    const Type& get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if written, `false` if read-only.
     */
    bool set_value(const Type& in_value) noexcept;
};

template<>
struct PropertyFactory<ml::vec4>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Vec4Property::Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const std::shared_ptr<const PropertyConstraint>&)
    {
        return std::make_unique<Vec4Property>(
          std::string{name},
          std::string{label},
          &value,
          offset,
          flags);
    }
};

template<>
struct PropertyFactory<ml::mat4x4>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Mat4Property::Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const std::shared_ptr<const PropertyConstraint>&)
    {
        return std::make_unique<Mat4Property>(
          std::string{name},
          std::string{label},
          &value,
          offset,
          flags);
    }
};

}    // namespace reflect

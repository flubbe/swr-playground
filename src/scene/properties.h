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

#include <ml/all.h>

#include "containers/string.h"
#include "reflection/property.h"

namespace reflect
{

#if SWR_CUSTOM_STRING_TYPE

/** Built-in reflected string property. */
class SwrStringProperty : public Property
{
public:
    using Type = swr::string;

private:
    /** Pointer to the reflected value. */
    Type* value{nullptr};

    /** Maximum accepted string length. */
    std::size_t max_length{256};

public:
    /**
     * Construct a string property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @param max_length Maximum accepted string length.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    SwrStringProperty(
      std::string_view name,
      std::string_view label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      std::size_t max_length = 256,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    const Type& get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if written, `false` if read-only.
     */
    bool set_value(std::string_view in_value);

    /** Return the maximum accepted string length. */
    std::size_t get_max_length() const noexcept;
};

template<>
struct PropertyFactory<swr::string>
{
    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      SwrStringProperty::Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>&)
    {
        return swr::make_unique<SwrStringProperty>(
          swr::string{name},
          swr::string{label},
          &value,
          offset,
          flags);
    }
};

#endif /* SWR_CUSTOM_STRING_TYPE */

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
      std::string_view name,
      std::string_view label,
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
    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Vec4Property::Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>&)
    {
        return swr::make_unique<Vec4Property>(
          name,
          label,
          &value,
          offset,
          flags);
    }
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
      std::string_view name,
      std::string_view label,
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
struct PropertyFactory<ml::mat4x4>
{
    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Mat4Property::Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>&)
    {
        return swr::make_unique<Mat4Property>(
          name,
          label,
          &value,
          offset,
          flags);
    }
};

}    // namespace reflect

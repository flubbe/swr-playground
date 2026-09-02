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
    /** Maximum accepted string length. */
    std::size_t max_length{256};

public:
    /**
     * Construct a string property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param max_length Maximum accepted string length.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    SwrStringProperty(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      std::size_t max_length = 256,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    /**
     * Construct a string property descriptor.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param max_length Maximum accepted string length.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    SwrStringProperty(
      std::string_view name,
      std::string_view label,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      std::size_t max_length = 256,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    void copy_value(
      void* dst,
      const void* src) const override;

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    const Type& get_value(const void* storage) const noexcept;

    /**
     * Set the current value.
     *
     * @param value New value.
     * @returns `true` if the new value was set.
     */
    bool set_value(void* storage, std::string_view value) const;

    /** Return the maximum accepted string length. */
    std::size_t get_max_length() const noexcept;

    static swr::unique_ptr<SwrStringProperty> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      std::size_t max_length = 256,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr)
    {
        return swr::make_unique<SwrStringProperty>(
          name,
          label,
          offset,
          element_count,
          flags,
          max_length,
          std::move(constraint));
    }
};

template<>
struct PropertyFactory<swr::string>
{
    using Type = swr::string;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>&)
    {
        return SwrStringProperty::construct(
          name,
          label,
          offset,
          element_count,
          flags);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          element_count,
          flags,
          constraint);
    }
};

#endif /* SWR_CUSTOM_STRING_TYPE */

/** Reflected 4D vector property. */
class Vec4Property : public Property
{
public:
    using Type = ml::vec4;

public:
    /**
     * Construct a 4D vector property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    Vec4Property(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None);

    /**
     * Construct a 4D vector property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    Vec4Property(
      std::string_view name,
      std::string_view label,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None);

    void copy_value(
      void* dst,
      const void* src) const override;

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    const Type& get_value(const void* storage) const noexcept;

    /**
     * Set the current value.
     *
     * @param value New value.
     * @returns `true` if the new value was set.
     */
    bool set_value(void* storage, const Type& value) const noexcept;

    static swr::unique_ptr<Vec4Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None)
    {
        return swr::make_unique<Vec4Property>(
          name,
          label,
          offset,
          element_count,
          flags);
    }
};

template<>
struct PropertyFactory<ml::vec4>
{
    using Type = ml::vec4;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>&)
    {
        return Vec4Property::construct(
          name,
          label,
          offset,
          element_count,
          flags);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          element_count,
          flags,
          constraint);
    }
};

/** Reflected 4x4 matrix property. */
class Mat4Property : public Property
{
public:
    using Type = ml::mat4x4;

public:
    /**
     * Construct a 4x4 matrix property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    Mat4Property(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None);

    /**
     * Construct a 4x4 matrix property descriptor.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    Mat4Property(
      std::string_view name,
      std::string_view label,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None);

    void copy_value(
      void* dst,
      const void* src) const override;

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    const Type& get_value(const void* storage) const noexcept;

    /**
     * Set the current value.
     *
     * @param value New value.
     * @returns `true` if the new value was set.
     */
    bool set_value(void* storage, const Type& value) const noexcept;

    static swr::unique_ptr<Mat4Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None)
    {
        return swr::make_unique<Mat4Property>(
          name,
          label,
          offset,
          element_count,
          flags);
    }
};

template<>
struct PropertyFactory<ml::mat4x4>
{
    using Type = ml::mat4x4;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>&)
    {
        return Mat4Property::construct(
          name,
          label,
          offset,
          element_count,
          flags);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          element_count,
          flags,
          constraint);
    }
};

}    // namespace reflect

/**
 * Software Rasterizer Playground.
 *
 * Built-in reflected properties.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <optional>

#include "property.h"

namespace reflect
{

/** Built-in reflected integer property. */
class IntProperty : public Property
{
public:
    using Type = int;

private:
    /** Pointer to the reflected value. */
    Type* value{nullptr};

    /** UI drag speed. */
    float speed{1.0f};

    /** Optional range constraint. */
    std::optional<RangeConstraint<Type>> range_constraint;

public:
    /**
     * Construct an integer property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    IntProperty(
      std::string name,
      std::string label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      float speed = 1.0f,
      std::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    Type get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if written, `false` if read-only.
     */
    bool set_value(Type in_value) noexcept;

    /** Return the UI drag speed. */
    float get_speed() const noexcept;
};

/** Built-in reflected unsigned integer property. */
class UIntProperty : public Property
{
public:
    using Type = unsigned int;

private:
    /** Pointer to the reflected value. */
    Type* value{nullptr};

    /** UI drag speed. */
    float speed{1.0f};

    /** Optional range constraint. */
    std::optional<RangeConstraint<Type>> range_constraint;

public:
    /**
     * Construct an unsigned integer property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    UIntProperty(
      std::string name,
      std::string label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      float speed = 1.0f,
      std::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    Type get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if written, `false` if read-only.
     */
    bool set_value(Type in_value) noexcept;

    /** Return the UI drag speed. */
    float get_speed() const noexcept;
};

/** Built-in reflected floating-point property. */
class FloatProperty : public Property
{
public:
    using Type = float;

private:
    /** Pointer to the reflected value. */
    Type* value{nullptr};

    /** UI drag speed. */
    float speed{0.01f};

    /** UI display format. */
    const char* format{"%.3f"};

    /** Optional range constraint. */
    std::optional<RangeConstraint<Type>> range_constraint;

public:
    /**
     * Construct a floating-point property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @param format UI display format.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    FloatProperty(
      std::string name,
      std::string label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      float speed = 0.01f,
      const char* format = "%.3f",
      std::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    Type get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if written, `false` if read-only.
     */
    bool set_value(Type in_value) noexcept;

    /** Return the UI drag speed. */
    float get_speed() const noexcept;

    /** Return the UI display format. */
    const char* get_format() const noexcept;
};

/** Built-in reflected boolean property. */
class BoolProperty : public Property
{
public:
    using Type = bool;

private:
    /** Pointer to the reflected value. */
    Type* value{nullptr};

public:
    /**
     * Construct a boolean property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    BoolProperty(
      std::string name,
      std::string label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      std::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    Type get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if written, `false` if read-only.
     */
    bool set_value(Type in_value) noexcept;
};

/** Built-in reflected string property. */
class StringProperty : public Property
{
public:
    using Type = std::string;

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
    StringProperty(
      std::string name,
      std::string label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      std::size_t max_length = 256,
      std::shared_ptr<const PropertyConstraint> constraint = nullptr);

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
struct PropertyFactory<int>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      int& value,
      std::size_t offset,
      PropertyFlags flags,
      const std::shared_ptr<const PropertyConstraint>& constraint)
    {
        return std::make_unique<IntProperty>(
          std::string{name},
          std::string{label},
          &value,
          offset,
          flags,
          1.0f,
          constraint);
    }
};

template<>
struct PropertyFactory<unsigned int>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      unsigned int& value,
      std::size_t offset,
      PropertyFlags flags,
      const std::shared_ptr<const PropertyConstraint>& constraint)
    {
        return std::make_unique<UIntProperty>(
          std::string{name},
          std::string{label},
          &value,
          offset,
          flags,
          1.0f,
          constraint);
    }
};

template<>
struct PropertyFactory<float>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      float& value,
      std::size_t offset,
      PropertyFlags flags,
      const std::shared_ptr<const PropertyConstraint>& constraint)
    {
        return std::make_unique<FloatProperty>(
          std::string{name},
          std::string{label},
          &value,
          offset,
          flags,
          0.01f,
          "%.3f",
          constraint);
    }
};

template<>
struct PropertyFactory<bool>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      bool& value,
      std::size_t offset,
      PropertyFlags flags,
      const std::shared_ptr<const PropertyConstraint>& constraint)
    {
        return std::make_unique<BoolProperty>(
          std::string{name},
          std::string{label},
          &value,
          offset,
          flags,
          constraint);
    }
};

template<>
struct PropertyFactory<std::string>
{
    static std::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::string& value,
      std::size_t offset,
      PropertyFlags flags,
      const std::shared_ptr<const PropertyConstraint>& constraint)
    {
        return std::make_unique<StringProperty>(
          std::string{name},
          std::string{label},
          &value,
          offset,
          flags,
          256,
          constraint);
    }
};

}    // namespace reflect

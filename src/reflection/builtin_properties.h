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
#include <filesystem>
#include <optional>
#include <vector>

#include "containers/format.h"
#include "reflection/property.h"

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
      std::string_view name,
      std::string_view label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      float speed = 1.0f,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    Type get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if the new value was set.
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
      std::string_view name,
      std::string_view label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      float speed = 1.0f,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    Type get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if the new value was set.
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
      std::string_view name,
      std::string_view label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      float speed = 0.01f,
      const char* format = "%.3f",
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    Type get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if the new value was set.
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
      std::string_view name,
      std::string_view label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    Type get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if the new value was set.
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
     * @returns `true` if the new value was set.
     */
    bool set_value(std::string_view in_value);

    /** Return the maximum accepted string length. */
    std::size_t get_max_length() const noexcept;
};

/** Element of a `VectorProperty`. */
class VectorElementProperty : public Property
{
    void* element_ptr{nullptr};

    using SetValueFn = void (*)(const void* source, void* destination);
    SetValueFn set_value_fn{nullptr};

public:
    template<typename T>
    VectorElementProperty(
      std::string_view name,
      std::string_view label,
      T* element_ptr,
      std::size_t offset,
      PropertyFlags flags,
      swr::shared_ptr<const PropertyConstraint> constraint)
    : Property{
        name,
        label,
        sizeof(T),
        offset,
        alignof(T),
        flags,
        std::move(constraint)}
    , element_ptr{element_ptr}
    , set_value_fn{[](const void* source, void* destination)
                   {
                       *static_cast<T*>(destination) = *static_cast<const T*>(source);
                   }}
    {
    }

    const void* get_type_tag() const noexcept override
    {
        return detail::type_tag<VectorElementProperty>();
    }

    void* get_value() noexcept
    {
        return element_ptr;
    }

    const void* get_value() const noexcept
    {
        return element_ptr;
    }

    bool set_value(const void* in_value)
    {
        if(!in_value
           || !element_ptr)
        {
            return false;
        }
        set_value_fn(in_value, element_ptr);
        return true;
    }
};

/** Built-in type-erased reflected vector property. */
class VectorProperty : public Property
{
    /** Type-erased pointer to the reflected value. */
    void* value{nullptr};

    /** Inner property info. */
    swr::unique_ptr<PropertyInfo> inner;

    using SetValueFn = void (*)(const void*, void*);
    SetValueFn set_value_fn;

    using SizeFn = std::size_t (*)(const void*);
    SizeFn size_fn;

    using ElementFn = void* (*)(void*, std::size_t);
    ElementFn element_fn;

    using ConstElementFn = const void* (*)(const void*, std::size_t);
    ConstElementFn const_element_fn;

    using MakeElementPropertyFn = swr::unique_ptr<Property> (*)(
      void* vector_ptr,
      std::size_t index,
      std::string_view name,
      std::string_view label,
      PropertyFlags flags);
    MakeElementPropertyFn make_element_fn{nullptr};

public:
    /**
     * Construct a vector property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    template<
      typename T,
      typename Alloc>
    VectorProperty(
      std::string_view name,
      std::string_view label,
      std::vector<T, Alloc>* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr)
    : Property{
        name,
        label,
        sizeof(std::vector<T, Alloc>),
        offset,
        alignof(std::vector<T, Alloc>),
        flags,
        std::move(constraint)}
    , value{value}
    , set_value_fn{
        [](const void* from, void* to) -> void
        {
            const auto& source =
              *static_cast<const std::vector<T, Alloc>*>(from);
            auto& destination =
              *static_cast<std::vector<T, Alloc>*>(to);
            destination = source;
        }}
    , size_fn{[](const void* p) -> std::size_t
              {
                  return static_cast<const std::vector<T, Alloc>*>(p)->size();
              }}
    , element_fn{[](void* p, std::size_t index) -> void*
                 {
                     return &static_cast<std::vector<T, Alloc>*>(p)->at(index);
                 }}
    , const_element_fn{[](const void* p, std::size_t index) -> const void*
                       {
                           return &static_cast<const std::vector<T, Alloc>*>(p)->at(index);
                       }}
    , make_element_fn{[](void* p, std::size_t i, std::string_view elem_name, std::string_view elem_label, PropertyFlags elem_flags) -> swr::unique_ptr<Property>
                      {
                          using MemberTraits = UnwrapType<T>;
                          using UnwrappedType = typename MemberTraits::ValueType;

                          T* elem_ptr = &static_cast<std::vector<T, Alloc>*>(p)->at(i);
                          UnwrappedType& unwrapped_value = MemberTraits::get(*elem_ptr);

                          // Construct typed Property (e.g. StringProperty, IntProperty)
                          // bound directly to the element pointer.
                          return PropertyFactory<UnwrappedType>::construct(
                            elem_name,
                            elem_label,
                            unwrapped_value,
                            0, /* offset */
                            elem_flags | PropertyFlags::ArrayElement,
                            {} /* constraints */
                          );
                      }}
    {
        if(value == nullptr)
        {
            throw std::invalid_argument{
              "VectorProperty requires non-null value pointer"};
        }

        using UnwrappedType = typename UnwrapType<T>::ValueType;
        inner = PropertyFactory<UnwrappedType>::construct_info(flags | PropertyFlags::ArrayElement);
    }

    const void*
      get_type_tag() const noexcept override;

    /** Return the current value. */
    const void* get_value() const noexcept
    {
        return &value;
    }

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if the new value was set.
     */
    bool set_value(const void* in_value)
    {
        set_value_fn(in_value, value);
        return true;
    }

    void* get_element(std::size_t index)
    {
        return element_fn(value, index);
    }

    const void* get_element(std::size_t index) const
    {
        return const_element_fn(value, index);
    }

    std::size_t get_length() const
    {
        return size_fn(value);
    }

    /** Construct typed `Property` (e.g., `StringProperty`) for element at `index`. */
    swr::unique_ptr<Property> get_element_property(
      std::size_t index) const
    {
        const swr::string index_str = swr::format("[{}]", index);
        return make_element_fn(value, index, index_str, index_str, get_flags());
    }

    /** Visit a specific element directly. */
    void accept_element(std::size_t index, PropertyVisitor& visitor)
    {
        auto elem_prop = get_element_property(index);
        elem_prop->accept(visitor);
    }
};

/** Built-in reflected path property. */
class PathProperty : public Property
{
public:
    using Type = std::filesystem::path;

private:
    /** Pointer to the reflected value. */
    Type* value{nullptr};

public:
    /**
     * Construct a path property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param value Pointer to the reflected value.
     * @param offset Byte offset from owning object base.
     * @param flags Property flags.
     * @throws `std::invalid_argument` if `value` is `nullptr`.
     */
    PathProperty(
      std::string_view name,
      std::string_view label,
      Type* value,
      std::size_t offset,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /** Return the current value. */
    const Type& get_value() const noexcept;

    /**
     * Set the current value.
     *
     * @param in_value New value.
     * @returns `true` if the new value was set.
     */
    bool set_value(const std::filesystem::path& in_value);
};

template<>
struct PropertyFactory<int>
{
    using Type = int;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return swr::make_unique<IntProperty>(
          name,
          label,
          &value,
          offset,
          flags,
          1.0f,
          constraint);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          flags,
          constraint);
    }
};

template<>
struct PropertyFactory<unsigned int>
{
    using Type = unsigned int;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return swr::make_unique<UIntProperty>(
          name,
          label,
          &value,
          offset,
          flags,
          1,
          constraint);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          flags,
          constraint);
    }
};

template<>
struct PropertyFactory<float>
{
    using Type = float;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return swr::make_unique<FloatProperty>(
          name,
          label,
          &value,
          offset,
          flags,
          0.01f,
          "%.3f",
          constraint);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          flags,
          constraint);
    }
};

template<>
struct PropertyFactory<bool>
{
    using Type = bool;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return swr::make_unique<BoolProperty>(
          name,
          label,
          &value,
          offset,
          flags,
          constraint);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          flags,
          constraint);
    }
};

template<>
struct PropertyFactory<std::string>
{
    using Type = std::string;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return swr::make_unique<StringProperty>(
          name,
          label,
          &value,
          offset,
          flags,
          256,
          constraint);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          flags,
          constraint);
    }
};

template<
  typename T,
  typename Alloc>
struct PropertyFactory<std::vector<T, Alloc>>
{
    using Type = std::vector<T, Alloc>;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return swr::make_unique<VectorProperty>(
          name,
          label,
          &value,
          offset,
          flags,
          constraint);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          flags,
          constraint);
    }
};

template<>
struct PropertyFactory<std::filesystem::path>
{
    using Type = std::filesystem::path;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      Type& value,
      std::size_t offset,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return swr::make_unique<PathProperty>(
          name,
          label,
          &value,
          offset,
          flags,
          constraint);
    }

    static swr::unique_ptr<PropertyInfo> construct_info(
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint = nullptr)
    {
        return swr::make_unique<PropertyInfo>(
          sizeof(Type),
          alignof(Type),
          flags,
          constraint);
    }
};

}    // namespace reflect

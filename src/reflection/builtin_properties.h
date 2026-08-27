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
class IntProperty
: public TypedProperty<int>
{
    /** UI drag speed. */
    Type speed{1};

    /** Optional range constraint. */
    std::optional<RangeConstraint<Type>> range_constraint;

public:
    /**
     * Construct an integer property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @param constraints Optional property constraints.
     */
    IntProperty(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      Type speed = 1,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    bool set_value(
      void* storage,
      const Type& value) const override;

    /** Return the UI drag speed. */
    Type get_speed() const noexcept;

    /**
     * Construct an integer property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @param constraints Optional property constraints.
     */
    static swr::unique_ptr<IntProperty> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      Type speed = 1,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);
};

/** Built-in reflected unsigned integer property. */
class UIntProperty
: public TypedProperty<unsigned int>
{
    /** UI drag speed. */
    Type speed{1u};

    /** Optional range constraint. */
    std::optional<RangeConstraint<Type>> range_constraint;

public:
    /**
     * Construct an unsigned integer property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @param constraints Optional property constraints.
     */
    UIntProperty(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      Type speed = 1u,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    bool set_value(
      void* storage,
      const Type& value) const override;

    const void* get_type_tag() const noexcept override;

    /** Return the UI drag speed. */
    Type get_speed() const noexcept;

    /**
     * Construct an unsigned integer property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @param constraints Optional property constraints.
     */
    static swr::unique_ptr<UIntProperty> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      Type speed = 1u,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);
};

/** Built-in reflected floating-point property. */
class FloatProperty
: public TypedProperty<float>
{
    /** UI drag speed. */
    Type speed{0.01f};

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
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @param format UI display format.
     * @param constraints Optional property constraints.
     */
    FloatProperty(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      Type speed = 0.01f,
      const char* format = "%.3f",
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    bool set_value(
      void* storage,
      const Type& value) const override;

    const void* get_type_tag() const noexcept override;

    /** Return the UI drag speed. */
    Type get_speed() const noexcept;

    /** Return the UI display format. */
    const char* get_format() const noexcept;

    /**
     * Construct a floating-point property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param speed UI drag speed.
     * @param format UI display format.
     * @param constraints Optional property constraints.
     */
    static swr::unique_ptr<FloatProperty> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      Type speed = 0.01f,
      const char* format = "%.3f",
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);
};

/** Built-in reflected boolean property. */
class BoolProperty
: public TypedProperty<bool>
{
public:
    /**
     * Construct a boolean property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param constraints Optional property constraints.
     */
    BoolProperty(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    const void* get_type_tag() const noexcept override;

    /**
     * Construct a boolean property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param constraints Optional property constraints.
     */
    static swr::unique_ptr<BoolProperty> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);
};

/** Built-in reflected string property. */
class StringProperty
: public TypedProperty<swr::string>
{
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
     * @param constraints Optional property constraints.
     */
    StringProperty(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      std::size_t max_length = 256,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    bool set_value(
      void* storage,
      const swr::string& value) const override;

    /**
     * Set the current value.
     *
     * @param value New value.
     * @returns `true` if the new value was set.
     */
    bool set_value(
      void* storage,
      std::string_view value) const;

    const void* get_type_tag() const noexcept override;

    /** Return the maximum accepted string length. */
    std::size_t get_max_length() const noexcept;

    /**
     * Construct a string property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param max_length Maximum accepted string length.
     * @param constraints Optional property constraints.
     */
    static swr::unique_ptr<StringProperty> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      std::size_t max_length = 256uz,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);
};

/** Built-in type-erased reflected vector property. */
class VectorProperty
: public Property
{
    /** Inner property info. */
    swr::unique_ptr<Property> inner;

    /** Assignment function type. */
    using AssignFn = void (*)(void*, const void*);

    /** Assignment function. */
    AssignFn assign_fn;

    /** Resize function type. */
    using ResizeFn = void (*)(void*, std::size_t);

    /** Resize function. */
    ResizeFn resize_fn;

    /** Element count function type. */
    using ElementCountFn = std::size_t (*)(const void*);

    /** Element count function. */
    ElementCountFn element_count_fn;

    /** Element getter type. */
    using ElementFn = void* (*)(void*, std::size_t);

    /** Element getter. */
    ElementFn element_fn;

    /** Constant element getter type. */
    using ConstElementFn = const void* (*)(const void*, std::size_t);

    /** Constant element getter. */
    ConstElementFn const_element_fn;

public:
    /**
     * Construct a vector property.
     *
     * @param size Size of the vector container.
     * @param alignment Alignment of the vector container.
     * @param assign_fn Assignment function between vectors.
     * @param resize_fn Resize function for the vector.
     * @param element_count_fn Function returning the element count of the container.
     * @param element_fn Function returning a writable element reference.
     * @param const_element_fn Function returning a read-only element reference.
     * @param inner Eleement property.
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param constraints Optional property constraints.
     */
    VectorProperty(
      std::size_t size,
      std::size_t alignment,
      AssignFn assign_fn,
      ResizeFn resize_fn,
      ElementCountFn element_count_fn,
      ElementFn element_fn,
      ConstElementFn const_element_fn,
      swr::unique_ptr<Property> inner,
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr)
    : Property{
        name,
        label,
        size,
        offset,
        alignment,
        element_count,
        flags,
        std::move(constraint)}
    , inner{std::move(inner)}
    , assign_fn{assign_fn}
    , resize_fn{resize_fn}
    , element_count_fn{element_count_fn}
    , element_fn{element_fn}
    , const_element_fn{const_element_fn}
    {
    }

    void copy_value(
      void* dst,
      const void* src) const override;

    const void* get_type_tag() const noexcept override;

    /**
     * Return the current value.
     *
     * @param storage Value storage.
     * @returns The current value.
     */
    const void* get_value(const void* storage) const noexcept
    {
        return storage;
    }

    /**
     * Set the current value.
     *
     * @param value New value.
     * @returns `true` if the new value was set.
     */
    bool set_value(
      void* storage,
      const void* value) const
    {
        assign_fn(storage, value);
        return true;
    }

    std::size_t get_element_count(
      const void* storage) const noexcept override
    {
        return element_count_fn(storage);
    }

    /** Resize the vector. */
    void resize(
      void* storage,
      std::size_t element_count) const
    {
        resize_fn(storage, element_count);
    }

    /** Get the inner property describing the elements. */
    const Property& get_inner() const
    {
        return *inner.get();
    }

    /**
     * Return the element at an index.
     *
     * @param storage Value storage.
     * @param index The element's index.
     * @returns Returns an element.
     */
    void* get_element(
      void* storage,
      std::size_t index) const
    {
        return element_fn(storage, index);
    }

    /**
     * Return the element at an index.
     *
     * @param storage Value storage.
     * @param index The element's index.
     * @returns Returns an element.
     */
    const void* get_element(
      const void* storage,
      std::size_t index) const
    {
        return const_element_fn(storage, index);
    }

    /**
     * Construct a vector property.
     *
     * @tparam T Element type.
     * @tparam Alloc Allocator of the vector container.
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param constraints Optional property constraints.
     */
    template<
      typename T,
      typename Alloc>
    static swr::unique_ptr<VectorProperty> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr)
    {
        using UnwrappedType = typename UnwrapType<T>::ValueType;
        auto inner = PropertyFactory<UnwrappedType>::construct(
          name,
          label,
          offset,
          element_count,
          flags,
          nullptr);    // TODO per-element constraints?

        return swr::make_unique<VectorProperty>(
          sizeof(std::vector<T, Alloc>),
          alignof(std::vector<T, Alloc>),
          [](void* dst, const void* src) -> void
          {
              auto& destination =
                *static_cast<std::vector<T, Alloc>*>(dst);
              const auto& source =
                *static_cast<const std::vector<T, Alloc>*>(src);
              destination = source;
          },
          [](void* p, std::size_t new_element_count) -> void
          {
              static_cast<std::vector<T, Alloc>*>(p)->resize(new_element_count);
          },
          [](const void* p) -> std::size_t
          {
              return static_cast<const std::vector<T, Alloc>*>(p)->size();
          },
          [](void* p, std::size_t index) -> void*
          {
              return &static_cast<std::vector<T, Alloc>*>(p)->at(index);
          },
          [](const void* p, std::size_t index) -> const void*
          {
              return &static_cast<const std::vector<T, Alloc>*>(p)->at(index);
          },
          std::move(inner),
          name,
          label,
          offset,
          element_count,
          flags,
          std::move(constraint));
    }
};

/** Built-in reflected path property. */
class PathProperty
: public TypedProperty<std::filesystem::path>
{
public:
    /**
     * Construct a path property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param constraints Optional property constraints.
     */
    PathProperty(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);

    bool set_value(
      void* storage,
      const std::filesystem::path& in_value) const override;

    const void* get_type_tag() const noexcept override;

    /**
     * Construct a path property.
     *
     * @param name Internal property name.
     * @param label Display name.
     * @param offset Byte offset from owning object base.
     * @param element_count Element count for arrays, or `0`.
     * @param flags Property flags.
     * @param constraints Optional property constraints.
     */
    static swr::unique_ptr<PathProperty> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags = PropertyFlags::None,
      swr::shared_ptr<const PropertyConstraint> constraint = nullptr);
};

template<>
struct PropertyFactory<int>
{
    using Type = int;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return IntProperty::construct(
          name,
          label,
          offset,
          element_count,
          flags,
          1,
          constraint);
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

template<>
struct PropertyFactory<unsigned int>
{
    using Type = unsigned int;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return UIntProperty::construct(
          name,
          label,
          offset,
          element_count,
          flags,
          1u,
          constraint);
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

template<>
struct PropertyFactory<float>
{
    using Type = float;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return FloatProperty::construct(
          name,
          label,
          offset,
          element_count,
          flags,
          0.01f,
          "%.3f",
          constraint);
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

template<>
struct PropertyFactory<bool>
{
    using Type = bool;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return BoolProperty::construct(
          name,
          label,
          offset,
          element_count,
          flags,
          constraint);
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

template<>
struct PropertyFactory<std::string>
{
    using Type = std::string;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return StringProperty::construct(
          name,
          label,
          offset,
          element_count,
          flags,
          256,
          constraint);
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

template<
  typename T,
  typename Alloc>
struct PropertyFactory<std::vector<T, Alloc>>
{
    using Type = std::vector<T, Alloc>;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return VectorProperty::construct<T, Alloc>(
          name,
          label,
          offset,
          element_count,
          flags,
          constraint);
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

template<>
struct PropertyFactory<std::filesystem::path>
{
    using Type = std::filesystem::path;

    static swr::unique_ptr<Property> construct(
      std::string_view name,
      std::string_view label,
      std::size_t offset,
      std::size_t element_count,
      PropertyFlags flags,
      const swr::shared_ptr<const PropertyConstraint>& constraint)
    {
        return PathProperty::construct(
          name,
          label,
          offset,
          element_count,
          flags,
          constraint);
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

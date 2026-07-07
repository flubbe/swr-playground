/**
 * Software Rasterizer Playground.
 *
 * Property reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cassert>
#include <optional>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "except.h"
#include "traits.h"

namespace reflect
{

/*
 * Forward declarations.
 */

class IntProperty;
class UIntProperty;
class FloatProperty;
class BoolProperty;
class StringProperty;
class Property;

/** Base class for property constraint metadata. */
struct PropertyConstraint
{
    /** Virtual destructor. */
    virtual ~PropertyConstraint() = default;

    /**
     * Return the runtime type tag for this constraint class.
     *
     * @note The tag is a unique memory address for the concrete type.
     */
    virtual const void* get_type_tag() const noexcept = 0;
};

/** Base class for property default value metadata. */
struct PropertyDefault
{
    /** Virtual destructor. */
    virtual ~PropertyDefault() = default;

    /**
     * Return the runtime type tag for this default class.
     *
     * @note The tag is a unique memory address for the concrete type.
     */
    virtual const void* get_type_tag() const noexcept = 0;
};

/**
 * Typed default value metadata.
 *
 * @tparam T Default value type.
 */
template<typename T>
struct TypedDefault : PropertyDefault
{
    /** Default value type. */
    using ValueType = T;

    /** Default value. */
    T value;

    /**
     * Construct a typed default value.
     *
     * @param value Default value.
     */
    explicit TypedDefault(T value)
    : value{std::move(value)}
    {
    }

    /** Return the runtime type tag for this default class. */
    const void* get_type_tag() const noexcept override;
};

/**
 * Create typed default metadata from a value.
 *
 * @tparam T Default value type.
 * @param value Default value.
 * @returns Shared metadata pointer containing `TypedDefault<T>`.
 */
template<typename T>
std::shared_ptr<const PropertyDefault> default_of(T&& value)
{
    using DefaultType = std::remove_cvref_t<T>;
    return std::make_shared<TypedDefault<DefaultType>>(
      std::forward<T>(value));
}

/**
 * Numeric range constraint metadata.
 *
 * @tparam T Constrained value type.
 */
template<typename T>
struct RangeConstraint : PropertyConstraint
{
    /** Constrained value type. */
    using ValueType = T;

    /** Optional minimum accepted value (inclusive). */
    std::optional<T> min;

    /** Optional maximum accepted value (inclusive). */
    std::optional<T> max;

    /** Optional UI step / increment hint. */
    std::optional<T> step;

    /** Whether values outside the range should be clamped instead of rejected. */
    bool clamp{false};

    /** Return the runtime type tag for this constraint class. */
    const void* get_type_tag() const noexcept override;
};

namespace detail
{

/**
 * Get a runtime tag for a type.
 *
 * @tparam T Type to tag.
 * @returns A unique memory address associated with `T`.
 */
template<typename T>
const void* type_tag() noexcept
{
    static const int tag = 0;
    return &tag;
}

/**
 * Concept for validating that a `Root` type supports `is_a(Owner::static_class())` for a given `Owner` type.
 *
 * @tparam Root The Root type to check.
 * @tparam Owner The Owner type whose static_class() is checked against Root's is_a
 */
template<
  typename Root,
  typename Owner>
concept RootSupportsIsA =
  requires(const Root* p) {
      { p->is_a(Owner::static_class()) } -> std::same_as<bool>;
  };

/**
 * Concept for validating that a `Root` type supports `is_a<T>()` for a given `Target` type.
 *
 * @tparam Root The Root type to check.
 * @tparam Target The typed target checked via `is_a<Target>()`.
 */
template<
  typename Root,
  typename Target>
concept RootSupportsTypedIsA =
  requires(const Root* p) {
      { p->template is_a<Target>() } -> std::same_as<bool>;
  };

}    // namespace detail

template<typename T>
const void* RangeConstraint<T>::get_type_tag() const noexcept
{
    return detail::type_tag<RangeConstraint<T>>();
}

template<typename T>
const void* TypedDefault<T>::get_type_tag() const noexcept
{
    return detail::type_tag<TypedDefault<T>>();
}

/** Visitor interface for properties. */
class PropertyVisitor
{
public:
    /** Virtual destructor. */
    virtual ~PropertyVisitor() = default;

    /** Visit a property. */
    virtual void visit(Property& property) = 0;
};

/** Base class for all properties. */
class Property
{
    /** Property name. */
    std::string name;

    /** Display name. */
    std::string label;

    /** Byte size of the reflected value type. */
    std::size_t size{0};

    /** Byte offset of the reflected value from the owning object base. */
    std::size_t offset{0};

    /** Alignment of the reflected value type in bytes. */
    std::size_t alignment{0};

    /** Property flags. */
    PropertyFlags flags{PropertyFlags::None};

    /** Optional typed constraint metadata. */
    std::shared_ptr<const PropertyConstraint> constraint{nullptr};

public:
    /**
     * Construct a property.
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     */
    Property(
      std::string name,
      std::string label,
      std::size_t size,
      std::size_t offset,
      std::size_t alignment,
      PropertyFlags flags = PropertyFlags::None,
      std::shared_ptr<const PropertyConstraint> constraint = nullptr)
    : name{std::move(name)}
    , label{std::move(label)}
    , size{size}
    , offset{offset}
    , alignment{alignment}
    , flags{flags}
    , constraint{std::move(constraint)}
    {
    }

    /** Virtual destructor. */
    virtual ~Property() = default;

    /** Property name. */
    const std::string& get_name() const noexcept
    {
        return name;
    }

    /** Property label / display name. */
    const std::string& get_label() const noexcept
    {
        return label;
    }

    /** Byte size of the reflected value type. */
    std::size_t get_size() const noexcept
    {
        return size;
    }

    /** Byte offset of the reflected value from the owning object base. */
    std::size_t get_offset() const noexcept
    {
        return offset;
    }

    /** Alignment of the reflected value type in bytes. */
    std::size_t get_alignment() const noexcept
    {
        return alignment;
    }

    /** Property flags. */
    PropertyFlags get_flags() const noexcept
    {
        return flags;
    }

    /** Optional typed constraint metadata. */
    const std::shared_ptr<const PropertyConstraint>& get_constraint() const noexcept
    {
        return constraint;
    }

    /**
     * Try to retrieve constraint metadata as `T`.
     *
     * @tparam T Constraint type.
     * @returns A pointer to `T` if the stored constraint exists and matches exactly, otherwise `nullptr`.
     */
    template<typename T>
    const T* try_get_constraint() const noexcept
    {
        static_assert(
          std::is_base_of_v<PropertyConstraint, T>,
          "T must derive from PropertyConstraint.");
        if(constraint == nullptr)
        {
            return nullptr;
        }
        if(constraint->get_type_tag() != detail::type_tag<T>())
        {
            return nullptr;
        }
        return static_cast<const T*>(constraint.get());
    }

    /**
     * Try to retrieve numeric range constraint metadata for value type `T`.
     *
     * @tparam T Constrained value type used by `RangeConstraint<T>`.
     * @returns A pointer to `RangeConstraint<T>` if the stored constraint exists and matches exactly,
     *          otherwise `nullptr`.
     */
    template<typename T>
    const RangeConstraint<T>* try_get_range_constraint() const noexcept
    {
        return try_get_constraint<RangeConstraint<T>>();
    }

    /**
     * Retrieve copied numeric range constraint metadata for `Type`.
     *
     * @tparam Type Constrained value type used by `RangeConstraint<Type>`.
     * @returns A copied `RangeConstraint<Type>` if present, otherwise `std::nullopt`.
     */
    template<typename Type>
    std::optional<RangeConstraint<Type>> range_constraint_from_metadata()
    {
        if(auto* constraint = try_get_range_constraint<Type>())
        {
            return *constraint;
        }

        return std::nullopt;
    }

    /** Whether the property is read-only. */
    bool is_read_only() const noexcept
    {
        return (get_flags() & PropertyFlags::ReadOnly) != PropertyFlags::None;
    }

    /** Return the runtime type tag for this property class. */
    virtual const void* get_type_tag() const noexcept = 0;

    /** Whether this property is of type `T`. */
    template<typename T>
    bool is_type() const noexcept
    {
        return get_type_tag() == detail::type_tag<T>();
    }

    /**
     * Try to cast this property to `T`
     *
     * @returns A pointer to `T` if the types match, and `nullptr` otherwise.
     */
    template<typename T>
    T* try_as() noexcept
    {
        if(!is_type<T>())
        {
            return nullptr;
        }
        return static_cast<T*>(this);
    }

    /**
     * Try to cast this property to `T`
     *
     * @returns A pointer to `T` if the types match, and `nullptr` otherwise.
     */
    template<typename T>
    const T* try_as() const noexcept
    {
        if(!is_type<T>())
        {
            return nullptr;
        }
        return static_cast<const T*>(this);
    }

    /**
     * Cast this property to `T`.
     *
     * @note Requires `is_type<T>() == true`. Only checked in debug builds.
     */
    template<typename T>
    T& as() noexcept
    {
        assert(is_type<T>());
        return *static_cast<T*>(this);
    }

    /**
     * Cast this property to `T`.
     *
     * @note Requires `is_type<T>() == true`. Only checked in debug builds.
     */
    template<typename T>
    const T& as() const noexcept
    {
        assert(is_type<T>());
        return *static_cast<const T*>(this);
    }

    /** Visitor acceptor. */
    void accept(PropertyVisitor& visitor)
    {
        visitor.visit(*this);
    }
};

/** Base class for property descriptors. */
struct DescriptorBase
{
    /** Property name. */
    std::string name;

    /** Display name. */
    std::string label;

    /** Property flags. */
    PropertyFlags flags;

    /** Optional typed constraint metadata. */
    std::shared_ptr<const PropertyConstraint> constraint;

    /** Optional typed default value metadata. */
    std::shared_ptr<const PropertyDefault> default_value;

    /**
     * Construct a property descriptor.
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     * @param constraint Optional constraint for the property values.
     * @param default_value Optional default value for the property.
     */
    DescriptorBase(
      std::string name,
      std::string label,
      const PropertyFlags flags,
      std::shared_ptr<const PropertyConstraint> constraint = nullptr,
      std::shared_ptr<const PropertyDefault> default_value = nullptr)
    : name{std::move(name)}
    , label{std::move(label)}
    , flags{flags}
    , constraint{std::move(constraint)}
    , default_value{std::move(default_value)}
    {
    }

    /** Whether the property is read-only. */
    bool is_read_only() const noexcept
    {
        return (flags & PropertyFlags::ReadOnly) != PropertyFlags::None;
    }

    /** Optional typed default value metadata. */
    const std::shared_ptr<const PropertyDefault>& get_default_value() const noexcept
    {
        return default_value;
    }

    /**
     * Try to retrieve default metadata as `TypedDefault<T>`.
     *
     * @tparam T Default value type.
     * @returns A pointer to `TypedDefault<T>` if the stored default exists and
     *          matches exactly, otherwise `nullptr`.
     */
    template<typename T>
    const TypedDefault<T>* try_get_default() const noexcept
    {
        if(default_value == nullptr)
        {
            return nullptr;
        }
        if(default_value->get_type_tag() != detail::type_tag<TypedDefault<T>>())
        {
            return nullptr;
        }
        return static_cast<const TypedDefault<T>*>(default_value.get());
    }
};

/** Property descriptor. */
struct PropertyDescriptor : DescriptorBase
{
    /** Function pointer type for constructing a property. */
    using ConstructFn = std::unique_ptr<Property> (*)(
      void*,
      std::string_view,
      std::string_view,
      PropertyFlags,
      const std::shared_ptr<const PropertyConstraint>&);

    /** Function pointer for constructing the property. */
    ConstructFn construct;

    /** Pointer to the next property descriptor in the descriptor linked list. */
    std::unique_ptr<PropertyDescriptor> next;

    /**
     * Construct a property descriptor.
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     * @param construct Function pointer for constructing the property.
     * @param next Pointer to the next property descriptor in the descriptor linked list.
     * @param contraint Optional constraint for the property values.
     * @param default_value Optional default value metadata.
     */
    PropertyDescriptor(
      std::string name,
      std::string label,
      const PropertyFlags flags,
      ConstructFn construct,
      std::unique_ptr<PropertyDescriptor> next,
      std::shared_ptr<const PropertyConstraint> constraint = nullptr,
      std::shared_ptr<const PropertyDefault> default_value = nullptr)
    : DescriptorBase{
        std::move(name),
        std::move(label),
        flags,
        std::move(constraint),
        std::move(default_value)}
    , construct{construct}
    , next{std::move(next)}
    {
    }
};

/**
 * Maps a C++ value type to a concrete `Property` implementation.
 *
 * Specialize this template for each reflected value type and provide:
 * `static std::unique_ptr<Property> construct(std::string_view, std::string_view, T&, std::size_t, PropertyFlags)`.
 */
template<typename T>
struct PropertyFactory;

/**
 * Type adapter used before property construction.
 *
 * By default, keeps `T` unchanged and returns the input reference.
 * Specialize to expose an underlying reflected type (e.g. wrappers/IDs) via `ValueType` and `get`.
 */
template<typename T>
struct UnwrapType
{
    using ValueType = T;

    static ValueType& get(T& value) noexcept
    {
        return value;
    }
};

namespace detail
{

/**
 * Convenience alias for the class type that owns a member pointer.
 *
 * Extracts `Class` from a pointer-to-member like `&Class::member`.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 */
template<auto MemberPtr>
using MemberClassType =
  typename MemberPointerTraits<decltype(MemberPtr)>::ClassType;

/**
 * Construct a property and bind it to a member.
 *
 * @note `obj` is statically typed as `MemberClassType<MemberPtr>&`, so
 *       type-correctness is enforced by the call site at compile time.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param obj Pointer to the object instance.
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 * @returns A unique pointer to the constructed property.
 */
template<auto MemberPtr>
std::unique_ptr<Property> construct_member(
  MemberClassType<MemberPtr>& obj,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  const std::shared_ptr<const PropertyConstraint>& constraint)
{
    using MemberPtrTraits = MemberPointerTraits<decltype(MemberPtr)>;
    using MemberType = typename MemberPtrTraits::MemberType;

    MemberType& value = obj.*MemberPtr;

    using MemberTraits = UnwrapType<MemberType>;
    using UnwrappedType = typename MemberTraits::ValueType;
    UnwrappedType& unwrapped_value = MemberTraits::get(value);
    const std::size_t property_offset = static_cast<std::size_t>(
      reinterpret_cast<std::uintptr_t>(std::addressof(unwrapped_value))
      - reinterpret_cast<std::uintptr_t>(std::addressof(obj)));

    return PropertyFactory<UnwrappedType>::construct(
      name,
      label,
      unwrapped_value,
      property_offset,
      flags,
      constraint);
}

/**
 * Construct a property and bind it to a member, with erased object type.
 *
 * @note Internal helper used by property descriptors.
 * @note Performs runtime type validation via the reflection hierarchy.
 *       If `obj` is not compatible with the member owner type, construction fails.
 * @note For reflected owner types, `obj` must be an erased pointer to the
 *       corresponding `Root` subobject.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param obj Pointer to the object instance (erased).
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 * @param constraint Constraint for the property values.
 * @returns A unique pointer to the constructed property.
 *
 * @throws `instance_error` If `obj` is `nullptr`.
 * @throws `instance_error` If `obj` does not point to a compatible owner type.
 */
template<auto MemberPtr>
std::unique_ptr<Property> construct_member_erased(
  void* obj,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  const std::shared_ptr<const PropertyConstraint>& constraint)
{
    if(obj == nullptr)
    {
        throw instance_error{"null object instance for property construction"};
    }

    using OwnerType = MemberClassType<MemberPtr>;
    OwnerType* owner_obj = nullptr;
    if constexpr(requires { typename OwnerType::Root; })
    {
        using RootType = typename OwnerType::Root;
        static_assert(
          RootSupportsIsA<RootType, OwnerType>,
          "OwnerType::Root must provide is_a(const ClassInfo*) for runtime type validation.");

        RootType* root_obj = static_cast<RootType*>(obj);
        if(!root_obj->is_a(OwnerType::static_class()))
        {
            throw instance_error{"object instance type mismatch for property construction"};
        }

        // Cast through RootType so inheritance pointer adjustment is applied correctly.
        owner_obj = static_cast<OwnerType*>(root_obj);
    }
    else
    {
        owner_obj = static_cast<OwnerType*>(obj);
    }

    return detail::construct_member<MemberPtr>(
      *owner_obj,
      name,
      label,
      flags,
      constraint);
}

}    // namespace detail

}    // namespace reflect

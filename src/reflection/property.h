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
#include <memory>
#include <string>
#include <string_view>

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

namespace detail
{

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
  requires(Root* p) {
      p->is_a(Owner::static_class());
  };

/**
 * Get a tag for a property type.
 *
 * @note The tag is a unique memory address.
 */
template<typename T>
const void* property_type_tag() noexcept
{
    static const int tag = 0;
    return &tag;
}

}    // namespace detail

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

    /** Property flags. */
    PropertyFlags flags{PropertyFlags::None};

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
      PropertyFlags flags = PropertyFlags::None)
    : name{std::move(name)}
    , label{std::move(label)}
    , flags{flags}
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

    /** Property flags. */
    PropertyFlags get_flags() const noexcept
    {
        return flags;
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
        return get_type_tag() == detail::property_type_tag<T>();
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

    /**
     * Construct a property descriptor.
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     */
    DescriptorBase(
      std::string name,
      std::string label,
      const PropertyFlags flags)
    : name{std::move(name)}
    , label{std::move(label)}
    , flags{flags}
    {
    }

    /** Whether the property is read-only. */
    bool is_read_only() const noexcept
    {
        return (flags & PropertyFlags::ReadOnly) != PropertyFlags::None;
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
      PropertyFlags);

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
     */
    PropertyDescriptor(
      std::string name,
      std::string label,
      const PropertyFlags flags,
      ConstructFn construct,
      std::unique_ptr<PropertyDescriptor> next)
    : DescriptorBase{
        std::move(name),
        std::move(label),
        flags}
    , construct{construct}
    , next{std::move(next)}
    {
    }
};

/**
 * Maps a C++ value type to a concrete `Property` implementation.
 *
 * Specialize this template for each reflected value type and provide:
 * `static std::unique_ptr<Property> construct(std::string_view, std::string_view, T&, PropertyFlags)`.
 */
template<typename T>
struct PropertyFactory;

/**
 * Type adapter used before property construction.
 *
 * By default, keeps `T` unchanged and returns the input reference.
 * Specialize to expose an underlying reflected type (e.g. wrappers/IDs) via `Type` and `get`.
 */
template<typename T>
struct UnwrapType
{
    using Type = T;

    static Type& get(T& value) noexcept
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
  PropertyFlags flags)
{
    using MemberPtrTraits = MemberPointerTraits<decltype(MemberPtr)>;
    using MemberType = typename MemberPtrTraits::MemberType;

    MemberType& value = obj.*MemberPtr;

    using MemberTraits = UnwrapType<MemberType>;
    using UnwrappedType = typename MemberTraits::Type;

    return PropertyFactory<UnwrappedType>::construct(
      name,
      label,
      MemberTraits::get(value),
      flags);
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
 * @return A unique pointer to the constructed property.
 *
 * @throws `instance_error` If `obj` is `nullptr`.
 * @throws `instance_error` If `obj` does not point to a compatible owner type.
 */
template<auto MemberPtr>
std::unique_ptr<Property> construct_member_erased(
  void* obj,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags)
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
      flags);
}

}    // namespace detail

}    // namespace reflect

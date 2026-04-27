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

#include <memory>
#include <string>
#include <string_view>

#include "ml/all.h"

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
class Mat4Property;

/** Visitor interface for properties. */
class PropertyVisitor
{
public:
    /** Virtual destructor. */
    virtual ~PropertyVisitor() = default;

    /** Visit an integer property. */
    virtual void visit(IntProperty& property) = 0;

    /** Visit an unsigned integer property. */
    virtual void visit(UIntProperty& property) = 0;

    /** Visit a floating-point property. */
    virtual void visit(FloatProperty& property) = 0;

    /** Visit a boolean property. */
    virtual void visit(BoolProperty& property) = 0;

    /** Visit a string property. */
    virtual void visit(StringProperty& property) = 0;

    /** Visit a 4x4 matrix property. */
    virtual void visit(Mat4Property& property) = 0;
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

    /** Visitor acceptor. */
    virtual void accept(PropertyVisitor& visitor) = 0;
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
 * @note `obj` has to be an instance of the class defined by `MemberPtr`, which is checked via the `is_a`
 *       method on the instance.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param obj Pointer to the object instance.
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 * @returns A unique pointer to the constructed property.
 *
 * @throws `instance_error` If `obj` is not an instance of the required class.
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
 * @note `obj` has to be a pointer to an instance of the class defined by `MemberPtr`, which is checked via the `is_a`
 *       method on the instance.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param obj Pointer to the object instance (erased).
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 * @return A unique pointer to the constructed property.
 *
 * @throws `instance_error` If `obj` is not a pointer to an instance of the required class.
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

    return construct_member<MemberPtr>(
      *static_cast<MemberClassType<MemberPtr>*>(obj),
      name,
      label,
      flags);
}

}    // namespace reflect

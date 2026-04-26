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

class IntProperty;
class UIntProperty;
class FloatProperty;
class BoolProperty;
class StringProperty;
class Mat4Property;

class PropertyVisitor
{
public:
    virtual ~PropertyVisitor() = default;

    virtual void visit(IntProperty& property) = 0;
    virtual void visit(UIntProperty& property) = 0;
    virtual void visit(FloatProperty& property) = 0;
    virtual void visit(BoolProperty& property) = 0;
    virtual void visit(StringProperty& property) = 0;
    virtual void visit(Mat4Property& property) = 0;
};

class Property
{
    /** Property name. */
    std::string name;

    /** Display name. */
    std::string label;

    /** Property flags. */
    PropertyFlags flags{PropertyFlags::None};

public:
    Property(
      std::string name,
      std::string label,
      PropertyFlags flags = PropertyFlags::None)
    : name{std::move(name)}
    , label{std::move(label)}
    , flags{flags}
    {
    }

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

struct DescriptorBase
{
    std::string name;
    std::string label;
    PropertyFlags flags;

    DescriptorBase(
      std::string name,
      std::string label,
      const PropertyFlags flags)
    : name{std::move(name)}
    , label{std::move(label)}
    , flags{flags}
    {
    }

    bool is_read_only() const
    {
        return (flags & PropertyFlags::ReadOnly) != PropertyFlags::None;
    }
};

struct PropertyDescriptor : DescriptorBase
{
    using ConstructFn = std::unique_ptr<Property> (*)(
      void*,
      std::string_view,
      std::string_view,
      PropertyFlags);

    ConstructFn construct;
    std::unique_ptr<PropertyDescriptor> next;

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

template<typename T>
struct PropertyFactory;

template<typename T>
struct UnwrapType
{
    using Type = T;

    static Type& get(T& value) noexcept
    {
        return value;
    }
};

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
 *
 * @throws instance_error If `obj` is not an instance of the required class.
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

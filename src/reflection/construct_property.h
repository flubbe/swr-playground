/**
 * Software Rasterizer Playground.
 *
 * Property construction.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <string_view>

#include "containers/memory.h"
#include "except.h"
#include "property.h"
#include "traits.h"

namespace reflect
{

/**
 * Maps a C++ value type to a concrete `Property` implementation.
 *
 * Specialize this template for each reflected value type and provide:
 * `static swr::unique_ptr<Property> construct(std::string_view, std::string_view, T&, std::size_t, PropertyFlags)`.
 */
template<typename T>
struct PropertyFactory;

/**
 * Type adapter used before property construction.
 *
 * By default, keeps `T` unchanged and returns the input reference.
 * Specialize to expose an underlying reflected type (e.g. wrappers/IDs) via `ValueType`.
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
swr::unique_ptr<Property> construct_member(
  MemberClassType<MemberPtr>& obj,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  const swr::shared_ptr<const PropertyConstraint>& constraint)
{
    using MemberPtrTraits = MemberPointerTraits<decltype(MemberPtr)>;
    using MemberType = typename MemberPtrTraits::MemberType;

    constexpr std::size_t element_count = std::extent_v<MemberType>;

    MemberType& value = obj.*MemberPtr;

    using MemberTraits = UnwrapType<MemberType>;
    using UnwrappedType = typename MemberTraits::ValueType;
    UnwrappedType& unwrapped_value = MemberTraits::get(value);
    const std::size_t property_offset = static_cast<std::size_t>(
      reinterpret_cast<const std::byte*>(std::addressof(unwrapped_value))
      - reinterpret_cast<const std::byte*>(std::addressof(obj)));

    return PropertyFactory<UnwrappedType>::construct(
      name,
      label,
      property_offset,
      element_count,
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
 * @throws `InstanceError` If `obj` is `nullptr`.
 * @throws `InstanceError` If `obj` does not point to a compatible owner type.
 */
template<auto MemberPtr>
swr::unique_ptr<Property> construct_member_erased(
  void* obj,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  const swr::shared_ptr<const PropertyConstraint>& constraint)
{
    if(obj == nullptr)
    {
        throw InstanceError{"null object instance for property construction"};
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
            throw InstanceError{"object instance type mismatch for property construction"};
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

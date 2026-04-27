/**
 * Software Rasterizer Playground.
 *
 * Reflection type traits.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <memory>
#include <string_view>

#include "flags.h"

namespace reflect
{

/*
 * Forward declarations.
 */

class Property;

/**
 * Reflection traits for a class, containing type information and function pointers for factory and property registration.
 *
 * This struct is used to define the traits for a class that participates in the reflection system. It contains type aliases
 * for the root class type and class info type, as well as function pointers for creating instances of the root class and
 * properties, and for registering properties with the class info.
 *
 * @tparam Root The root class type (base class) for the reflection hierarchy.
 * @tparam Info The class info type used for registration.
 */
template<
  typename Root,
  typename Info>
struct ReflectionTraits
{
    using RootType = Root;
    using ClassInfoType = Info;

    using FactoryFn = std::unique_ptr<Root> (*)();
    using ConstructFn = std::unique_ptr<Property> (*)(
      Root&,
      std::string_view,
      std::string_view,
      PropertyFlags);
    using PropertyRegisterFn = void (*)(ClassInfoType&);
};

/**
 * Primary template intentionally left undefined.
 *
 * Specializations expose `ClassType` and `MemberType` for pointer-to-member types (`Member Class::*`).
 */
template<typename T>
struct MemberPointerTraits;

/**
 * Helper to get class and member types.
 *
 * This struct is a template specialization for member pointer types (e.g., `Member Class::*`). It defines type aliases for
 * the class type and member type, allowing you to extract this information from a member pointer. This is useful in the
 * reflection system for registering properties and accessing member variables.
 *
 * @tparam Class The class type that contains the member.
 * @tparam Member The type of the member variable.
 */
template<
  typename Class,
  typename Member>
struct MemberPointerTraits<Member Class::*>
{
    using ClassType = Class;
    using MemberType = Member;
};

}    // namespace reflect

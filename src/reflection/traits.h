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

#include "flags.h"

namespace reflect
{

class Property;

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

/** Helper to get class and member types. */
template<typename T>
struct MemberPointerTraits;

/** Helper to get class and member types. */
template<typename Class, typename Member>
struct MemberPointerTraits<Member Class::*>
{
    using ClassType = Class;
    using MemberType = Member;
};

}    // namespace reflect

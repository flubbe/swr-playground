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

namespace reflect
{

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

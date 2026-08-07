/**
 * Software Rasterizer Playground.
 *
 * Runtime-checked casts.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <memory>
#include <type_traits>

#include "class_registry.h"
#include "except.h"
#include "property.h"

namespace reflect
{

/**
 * Try to cast a reflected root pointer to a derived reflected type.
 *
 * @tparam TargetType The requested cast target.
 * @tparam RootType The reflected root hierarchy type.
 * @param instance Pointer to the root subobject.
 * @returns Pointer to `TargetType` on success, `nullptr` on null or mismatch.
 */
template<
  typename TargetType,
  typename RootType>
    requires std::is_base_of_v<
               std::remove_cvref_t<RootType>,
               std::remove_cvref_t<TargetType>>
             && detail::RootSupportsTypedIsA<
               std::remove_cvref_t<RootType>,
               std::remove_cvref_t<TargetType>>
auto* try_cast(
  RootType* instance) noexcept
{
    using NormalizedTargetType = std::remove_cvref_t<TargetType>;
    using ResultType = std::conditional_t<
      std::is_const_v<RootType>,
      std::add_const_t<NormalizedTargetType>,
      NormalizedTargetType>;

    if(instance == nullptr
       || !instance->template is_a<NormalizedTargetType>())
    {
        return static_cast<ResultType*>(nullptr);
    }
    return static_cast<ResultType*>(instance);
}

/**
 * Cast a reflected root reference to a derived reflected type.
 *
 * @tparam TargetType The requested cast target.
 * @tparam RootType The reflected root hierarchy type (constness is preserved).
 * @param instance Reference to the root subobject.
 * @returns Reference to `TargetType` (const-qualified when `instance` is const).
 * @throws `InstanceError` If runtime type check fails.
 */
template<typename TargetType, typename RootType>
    requires std::is_base_of_v<
               std::remove_cvref_t<RootType>,
               std::remove_cvref_t<TargetType>>
             && detail::RootSupportsTypedIsA<
               std::remove_cvref_t<RootType>,
               std::remove_cvref_t<TargetType>>
auto& cast(
  RootType& instance)
{
    using NormalizedTargetType = std::remove_cvref_t<TargetType>;
    using ResultType = std::conditional_t<
      std::is_const_v<RootType>,
      std::add_const_t<NormalizedTargetType>,
      NormalizedTargetType>;

    auto* casted = try_cast<TargetType>(std::addressof(instance));
    if(casted == nullptr)
    {
        throw InstanceError{"object instance type mismatch for cast"};
    }
    return static_cast<ResultType&>(*casted);
}

}    // namespace reflect

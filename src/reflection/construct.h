/**
 * Software Rasterizer Playground.
 *
 * Reflected type construction.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <string_view>

#include "class_info.h"
#include "class_registry.h"
#include "cast.h"
#include "except.h"

namespace reflect
{

/** Call the class' destroy function (`delete`). */
struct ReflectedDeleter
{
    /** Class info. */
    const ClassInfo* cls{nullptr};

    void operator()(void* ptr) const noexcept
    {
        if(ptr != nullptr
           && cls != nullptr)
        {
            cls->destroy(ptr);
        }
    }
};

/** Unique pointer with custom deleter. */
template<typename T>
using unique_ptr = std::unique_ptr<
  T,
  ReflectedDeleter>;

/**
 * Construct an instance from a `ClassInfo` descriptor.
 *
 * @tparam Root
 * @param cls Pointer to the class metadata.
 * @returns Unique pointer to the constructed instance, or `nullptr` if
 *     either `cls` is `nullptr` or has it no factory function.
 * @throws Throws a `InstanceError` if the hierarchy root tag does not match `Root`.
 * @throws Throws a `InstanceError` if `cls` is not a subclass of `Root::static_class()`.
 * @throws Throws a `InstanceError` if the constructed object is not a `Root`.
 */
template<typename Root>
unique_ptr<Root> construct(
  const ClassInfo* cls)
{
    if(cls == nullptr
       || cls->factory == nullptr)
    {
        return {};
    }

    if(cls->root_tag != detail::root_type_tag<Root>())
    {
        throw InstanceError{"ClassInfo hierarchy root tag does not match requested Root type."};
    }
    if(!cls->is_a(Root::static_class()))
    {
        throw InstanceError{"ClassInfo is not a subclass of requested Root."};
    }

    auto* raw_instance = cls->factory();
    auto* root_instance = static_cast<Root*>(raw_instance);

    // Post-construction sanity check: Verify the object's embedded metadata matches
    if(root_instance != nullptr
       && !root_instance->is_a(cls))
    {
        if(cls->destroy != nullptr)
        {
            cls->destroy(raw_instance);
        }
        throw InstanceError{"Constructed instance metadata does not match ClassInfo."};
    }

    return unique_ptr<Root>{
      root_instance,
      ReflectedDeleter{cls}};
}

/**
 * Construct an instance.
 *
 * @tparam Root Root type for the class hierarchy.
 * @param qualified_name Qualified class name (e.g., "module.ClassName").
 * @returns Unique pointer to the constructed instance.
 * @throws Throws a `InstanceError` if the `qualified_name` is not found in the reflection system.
 * @throws Throws a `InstanceError` if the hierarchy root tag does not match `Root`.
 * @throws Throws a `InstanceError` if `qualified_name` is not a subclass of `Root::static_class()`.
 * @throws Throws a `InstanceError` if the constructed object is not a `Root`.
 */
template<typename Root>
unique_ptr<Root> construct(
  std::string_view qualified_name)
{
    const ClassInfo* cls = ReflectionSystem::find_class(
      qualified_name,
      detail::root_type_tag<Root>());

    if(cls == nullptr)
    {
        throw InstanceError{
          std::format(
            "class '{}' not found in reflection system",
            qualified_name)};
    }

    return construct<Root>(cls);
}

/**
 * Construct and initialize an instance.
 *
 * @tparam Root Root type for the class hierarchy.
 * @tparam Class Class to construct.
 * @tparam Args Initialization parameters. Forwarded to `Class::init` if the method exists.
 *     Must be empty if `Class::init` does not exist.
 * @returns Unique pointer to the constructed instance.
 * @throws Throws a `InstanceError` if the hierarchy root tag does not match `Root`.
 * @throws Throws a `InstanceError` if the constructed object is not a `Root`.
 */
template<
  typename Root,
  typename Class,
  typename... Args>
    requires(
      sizeof...(Args) == 0
      || requires(Class* c, Args&&... args) {
             c->init(std::forward<Args>(args)...);
         })
unique_ptr<Class> construct_and_init(Args&&... args)
{
    unique_ptr<Root> obj = construct<Root>(Class::static_class());
    auto* ptr = static_cast<Class*>(obj.get());

    if constexpr(requires { ptr->init(std::forward<Args>(args)...); })
    {
        ptr->init(std::forward<Args>(args)...);
    }

    auto deleter = obj.get_deleter();
    obj.release();

    return unique_ptr<Class>{ptr, deleter};
}

}    // namespace reflect

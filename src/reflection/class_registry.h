/**
 * Software Rasterizer Playground.
 *
 * Reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>

#include "class_info.h"

namespace reflect
{

/** Static info about a pending class registration. */
struct PendingClassRegistration
{
    /** Module name of the class. */
    std::string_view module_name{};

    /** The class name. */
    std::string_view name{};

    /** Byte size of the class. */
    std::size_t size{0};

    /** Pointer to the static `ClassInfo` instance/storage. */
    ClassInfo* storage{nullptr};

    /** The super class. */
    ClassInfo* super{nullptr};

    /** Instance creation. */
    FactoryFn factory{nullptr};

    /** Property registration. */
    PropertyRegisterFn register_properties{nullptr};
};

/** Linked-list for the automatic registration. */
struct PendingClassNode
{
    /** Pending class registration. */
    const PendingClassRegistration* reg{nullptr};

    /** Next pending node. */
    PendingClassNode* next{nullptr};
};

/** Reflection system. */
class ReflectionSystem
{
    friend struct AutoClassRegistrar;

    /**
     * Add a class to the pending registration list.
     *
     * @param node The class info inside a class node.
     */
    static void add_pending(
      PendingClassNode* node) noexcept;

public:
    /**
     * Enable/disable automatic class registration. Useful when loading dynamic libraries.
     * Registration is enabled by default.
     *
     * @param enabled Whether to enable automatic registration.
     */
    static void allow_auto_registration(
      bool enabled) noexcept;

    /** Whether automatic registration is currently allowed. */
    static bool is_auto_registration_allowed() noexcept;

    /**
     * Process all pending registrations. Needs to be called after
     * automatic registration ended, e.g. after dynamic library loading.
     *
     * @throws Throws `std::runtime_error` if called while automatic registration is enabled.
     * @throws Throws `std::runtime_error` if the registration data is invalid.
     * @throws Throws `std::runtime_error` if a class was already registered.
     */
    static void process_pending_registrations();

    /**
     * Find a class by name.
     *
     * @param qualified_name The qualified class name as `module_name.class_name`.
     * @returns Returns the class info, or `nullptr` if the class is not found.
     */
    static const ClassInfo* find_class(
      std::string_view qualified_name);

    /**
     * Remove a class from the registry.
     *
     * @param qualified_name The qualified class name.
     * @returns Returns `bool` if `qualified_name` was removed from the registry.
     */
    static bool unregister_class(
      std::string_view qualified_name);

    /**
     * Remove all classes loaded by a module.
     *
     * @param module_name Name of the module to remove.
     * @returns Returns the removed class count.
     */
    static std::size_t unregister_module(
      std::string_view module_name);

    /** Clear the registry. */
    static void clear();
};

/** Automatic class registration helper. Executed during static initialization. */
struct AutoClassRegistrar
{
    /**
     * Register a class node.
     *
     * @param node The node to add.
     */
    explicit AutoClassRegistrar(
      PendingClassNode* node) noexcept
    {
        ReflectionSystem::add_pending(node);
    }
};

/** Factory for object creation. */
template<typename T>
std::unique_ptr<Object> factory()
{
    static_assert(
      std::is_base_of_v<Object, T>,
      "T must inherit from Object");
    return std::make_unique<T>();
}

/** Return a type's super class or `nullptr` if there is none. */
template<typename T>
ClassInfo* super_class_info() noexcept
{
    if constexpr(requires { typename T::Super; })
    {
        return T::Super::static_class();
    }

    return nullptr;
}

/** Templated reflection type entry. */
template<typename T>
struct TypeReflection;

/** Registration helper for statically registered classes. */
template<typename T>
struct StaticClassRegistration
{
    static_assert(
      std::is_convertible_v<decltype(TypeReflection<T>::module_name), std::string_view>,
      "TypeReflection<T>::module_name must be convertible to std::string_view");
    static_assert(
      std::is_convertible_v<decltype(TypeReflection<T>::class_name), std::string_view>,
      "TypeReflection<T>::class_name must be convertible to std::string_view");
    static_assert(
      std::is_same_v<decltype(TypeReflection<T>::register_properties), const PropertyRegisterFn>,
      "TypeReflection<T>::register_properties must be PropertyRegisterFn");

    ClassInfo storage{};
    PendingClassRegistration registration{
      .module_name = TypeReflection<T>::module_name,
      .name = TypeReflection<T>::class_name,
      .size = sizeof(T),
      .storage = &storage,
      .super = super_class_info<T>(),
      .factory = &factory<T>,
      .register_properties = TypeReflection<T>::register_properties};
    PendingClassNode node{
      .reg = &registration,
      .next = nullptr};
    AutoClassRegistrar registrar{&node};
};

/** Return the static class registration for a type. */
template<typename T>
StaticClassRegistration<T>& class_registration() noexcept;

/**
 * Reflection declaration/definition pattern:
 * - Put `DECLARE_REFLECTION(Module, Type)` in the type header.
 * - Put `DEFINE_REFLECTION(Type)` in exactly one translation unit.
 */
#define DECLARE_REFLECTION(Module, Type)                                \
    namespace reflect                                                   \
    {                                                                   \
    template<>                                                          \
    struct TypeReflection<Type>                                         \
    {                                                                   \
        static constexpr std::string_view module_name = #Module;        \
        static constexpr std::string_view class_name = #Type;           \
        static constexpr PropertyRegisterFn register_properties =       \
          &Type::register_properties;                                   \
    };                                                                  \
    template<>                                                          \
    StaticClassRegistration<Type>& class_registration<Type>() noexcept; \
    }

#define DEFINE_REFLECTION(Type)                                                 \
    namespace                                                                   \
    {                                                                           \
    reflect::StaticClassRegistration<Type> g_reflection_registration_##Type{};  \
    }                                                                           \
    namespace reflect                                                           \
    {                                                                           \
    template<>                                                                  \
    reflect::StaticClassRegistration<Type>& class_registration<Type>() noexcept \
    {                                                                           \
        return g_reflection_registration_##Type;                                \
    }                                                                           \
    }

/** Super class for the root class of all reflected classes. */
template<typename Derived>
class ReflectRoot
{
public:
    static ClassInfo* static_class() noexcept
    {
        return &class_registration<Derived>().storage;
    }
};

/** Super class for a non-root reflected class. */
template<typename Derived, typename Base>
class Reflected : public Base
{
public:
    using Super = Base;
    using Base::Base;

    static ClassInfo* static_class() noexcept
    {
        return &class_registration<Derived>().storage;
    }

    const ClassInfo* get_class() const override
    {
        return Derived::static_class();
    }
};

/*
 * Property registration.
 */

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

/**
 * Register a data member as a reflected property.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param class_info Class metadata to append the property to.
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 */
template<auto MemberPtr>
void register_property(
  ClassInfo& class_info,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags)
{
    using Traits = MemberPointerTraits<decltype(MemberPtr)>;
    using ClassType = typename Traits::ClassType;
    using MemberType = typename Traits::MemberType;

    auto descriptor = std::make_unique<PropertyDescriptor>(
      name,
      label,
      flags,
      &construct_member<ClassType, MemberType, MemberPtr>,
      std::move(class_info.first_property));
    class_info.first_property = std::move(descriptor);
}

}    // namespace reflect

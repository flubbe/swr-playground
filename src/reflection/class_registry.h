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
#include <vector>

#include "class_info.h"

namespace reflect
{

/**
 * Get a tag for a type.
 *
 * @note The tag is a unique memory address.
 */
template<typename Root>
const void* root_type_tag() noexcept
{
    static const int tag = 0;
    return &tag;
}

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

    /** Root hierarchy marker for this class. */
    const void* root_tag{nullptr};

    /** Instance creation (erased). */
    ClassInfo::FactoryFn factory{nullptr};

    /** Instance destruction (erased). */
    ClassInfo::DestroyFn destroy{nullptr};

    /** Property registration. */
    ClassInfo::PropertyRegisterFn register_properties{nullptr};
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
     * Find a class by name within a specific root hierarchy marker.
     *
     * @param qualified_name The qualified class name as `module_name.class_name`.
     * @param root_tag Root hierarchy marker.
     * @returns Returns the class info if found and root-compatible, or `nullptr`.
     */
    static const ClassInfo* find_class(
      std::string_view qualified_name,
      const void* root_tag);

    /**
     * Find a class by name for a specific root hierarchy.
     *
     * @tparam Root Root type used to filter classes.
     *
     * @param qualified_name The qualified class name as `module_name.class_name`.
     * @returns Returns the class info if found and root-compatible, or `nullptr`.
     */
    template<typename Root>
    static const ClassInfo* find_class(
      std::string_view qualified_name)
    {
        return find_class(
          qualified_name,
          root_type_tag<Root>());
    }

    /**
     * Remove a class from the registry for a specific root hierarchy marker.
     *
     * @param qualified_name The qualified class name.
     * @param root_tag Root hierarchy marker.
     * @returns Returns `true` if a class matching `qualified_name` and `root_tag` was removed.
     */
    static bool unregister_class(
      std::string_view qualified_name,
      const void* root_tag);

    /**
     * Remove a class from the registry for a specific root hierarchy type.
     *
     * @tparam Root Root type used to filter classes.
     *
     * @param qualified_name The qualified class name.
     * @returns Returns `true` if a root-compatible class was removed.
     */
    template<typename Root>
    static bool unregister_class(
      std::string_view qualified_name)
    {
        return unregister_class(
          qualified_name,
          root_type_tag<Root>());
    }

    /**
     * Remove all classes loaded by a module.
     *
     * @param module_name Name of the module to remove.
     * @returns Returns the removed class count.
     */
    static std::size_t unregister_module(
      std::string_view module_name);

    /**
     * Return all currently registered classes.
     *
     * @returns Returns a stable, sorted snapshot of registered classes.
     */
    static std::vector<const ClassInfo*> get_registered_classes();

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

/**
 * Factory for `T`.
 *
 * @tparam Root Root type for the class hierarchy.
 * @tparam T The type to construct.
 * @returns Returns a new (type-erased) instance of `T`.
 */
template<
  typename Root,
  typename T>
void* factory()
{
    static_assert(
      std::is_base_of_v<Root, T>,
      "T must inherit from root type");
    return static_cast<Root*>(new T{});
}

/**
 * Destroy an instance of a child class of `Root`.
 *
 * @tparam Root Root type for the class hierarchy.
 * @param instance The instance to destroy.
 */
template<typename Root>
void destroy(
  void* instance)
{
    static_assert(
      std::has_virtual_destructor_v<Root>,
      "Root must have a virtual destructor");
    delete static_cast<Root*>(instance);
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
template<
  typename Root,
  typename T>
struct TypeReflection;

/**
 * Registration helper for statically registered classes.
 *
 * @tparam Root Root type for the class hierarchy.
 * @tparam T The type to register.
 */
template<
  typename Root,
  typename T>
struct StaticClassRegistration
{
    static_assert(
      std::is_convertible_v<
        decltype(TypeReflection<Root, T>::module_name),
        std::string_view>,
      "TypeReflection<T>::module_name must be convertible to std::string_view");
    static_assert(
      std::is_convertible_v<
        decltype(TypeReflection<Root, T>::class_name),
        std::string_view>,
      "TypeReflection<T>::class_name must be convertible to std::string_view");
    static_assert(
      std::is_same_v<
        decltype(TypeReflection<Root, T>::register_properties),
        const ClassInfo::PropertyRegisterFn>,
      "TypeReflection<T>::register_properties must be PropertyRegisterFn");

    ClassInfo storage{};
    PendingClassRegistration registration{
      .module_name = TypeReflection<Root, T>::module_name,
      .name = TypeReflection<Root, T>::class_name,
      .size = sizeof(T),
      .storage = &storage,
      .super = super_class_info<T>(),
      .root_tag = root_type_tag<Root>(),
      .factory = &factory<Root, T>,
      .destroy = &destroy<Root>,
      .register_properties = TypeReflection<Root, T>::register_properties};
    PendingClassNode node{
      .reg = &registration,
      .next = nullptr};
    AutoClassRegistrar registrar{&node};
};

/** Return the static class registration for a type. */
template<typename Root, typename T>
StaticClassRegistration<Root, T>& class_registration() noexcept;

/**
 * Declare a reflected type.
 *
 * Reflection declaration/definition pattern:
 * - Put `DECLARE_REFLECTION(Module, Type)` in the type header.
 * - Put `DEFINE_REFLECTION(Type)` in exactly one translation unit.
 */
#define DECLARE_REFLECTION(Module, Type)                                                        \
    namespace reflect                                                                           \
    {                                                                                           \
    template<>                                                                                  \
    struct TypeReflection<Type::Root, Type>                                                     \
    {                                                                                           \
        static constexpr std::string_view module_name = #Module;                                \
        static constexpr std::string_view class_name = #Type;                                   \
        static constexpr auto register_properties =                                             \
          &Type::register_properties;                                                           \
    };                                                                                          \
    template<>                                                                                  \
    StaticClassRegistration<Type::Root, Type>& class_registration<Type::Root, Type>() noexcept; \
    }

/**
 * Define a reflected type.
 *
 * Reflection declaration/definition pattern:
 * - Put `DECLARE_REFLECTION(Module, Type)` in the type header.
 * - Put `DEFINE_REFLECTION(Type)` in exactly one translation unit.
 */
#define DEFINE_REFLECTION(Type)                                                            \
    namespace                                                                              \
    {                                                                                      \
    reflect::StaticClassRegistration<Type::Root, Type> g_reflection_registration_##Type{}; \
    }                                                                                      \
    namespace reflect                                                                      \
    {                                                                                      \
    template<>                                                                             \
    reflect::StaticClassRegistration<Type::Root, Type>&                                    \
      class_registration<Type::Root, Type>() noexcept                                      \
    {                                                                                      \
        return g_reflection_registration_##Type;                                           \
    }                                                                                      \
    }

/**
 * Super class for the root class of all reflected classes.
 *
 * Usage example:
 * ```cpp
 * class Object : public ReflectRoot<Object>
 * {};
 * ```
 *
 * @tparam Base The root class.
 */
template<typename Base>
class ReflectRoot
{
public:
    using Root = Base;

    /**
     * Returns the reflection metadata for this type.
     *
     * The returned pointer refers to the statically registered `ClassInfo`
     * instance describing this class in the reflection system.
     * The pointer is valid for the lifetime of the program.
     */
    static ClassInfo* static_class() noexcept
    {
        return &class_registration<Root, Root>().storage;
    }
};

/**
 * Concept for base classes in a reflected type hierarchy.
 *
 * A reflected base must provide a nested `Root` type and expose
 * `get_class()` on const instances, returning exactly `const ClassInfo*`.
 *
 * @note This does not check that `get_class()` is virtual. That is enforced
 *       by `override` in derived reflected classes.
 */
template<typename T>
concept ReflectedBase =
  requires {
      typename T::Root;
  } && requires(const T& t) {
      { t.get_class() } -> std::same_as<const ClassInfo*>;
  };

/**
 * CRTP base for a non-root reflected class.
 *
 * `Reflected<Derived, Base>` inherits from `Base` and provides reflection
 * metadata for `Derived`.
 *
 * Usage example:
 * ```cpp
 * class Chair : public Reflected<Chair, Object>
 * {};
 * ```
 *
 * @tparam Derived The concrete reflected class.
 * @tparam Base The reflected base class. Must provide `Root`, `static_class()`,
 *              and a virtual `get_class()` override target.
 */
template<
  typename Derived,
  ReflectedBase Base>
class Reflected : public Base
{
public:
    using Super = Base;
    using Root = typename Base::Root;
    using Base::Base;

    /**
     * Returns the reflection metadata for this type.
     *
     * The returned pointer refers to the statically registered `ClassInfo`
     * instance describing this class in the reflection system.
     * The pointer is valid for the lifetime of the program.
     */
    static ClassInfo* static_class() noexcept
    {
        return &class_registration<Root, Derived>().storage;
    }
    /**
     * Returns the reflection metadata for this instance.
     *
     * This is the virtual, instance-level counterpart to `static_class()`,
     * allowing access to the concrete type's `ClassInfo` through a base
     * class pointer or reference.
     */
    const ClassInfo* get_class() const override
    {
        return Derived::static_class();
    }
};

/*
 * Property registration.
 */

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
  PropertyFlags flags = PropertyFlags::None)
{
    auto descriptor = std::make_unique<
      PropertyDescriptor>(
      std::string{name},
      std::string{label},
      flags,
      &construct_member_erased<MemberPtr>,
      std::move(class_info.first_property));
    class_info.first_property = std::move(descriptor);
}

}    // namespace reflect

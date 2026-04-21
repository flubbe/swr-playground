/**
 * Software Rasterizer Playground.
 *
 * Object reflection.
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

/** Static info about a pending class registration. */
struct PendingClassRegistration
{
    /** The class name. */
    std::string_view name{};

    /** Module name of the class. */
    std::string_view module_name{};

    /** Byte size of the class. */
    std::size_t size{0};

    /** Pointer to the static `ClassInfo` instance/storage. */
    ClassInfo* storage{nullptr};

    /** Return the base class. */
    const ClassInfo* (*get_base_class)() noexcept = nullptr;

    /** Instance creation. */
    FactoryFn factory{nullptr};
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
    static const ClassInfo* find_class(std::string_view qualified_name);

    /**
     * Remove a class from the registry.
     *
     * @param qualified_name The qualified class name.
     * @returns Returns `bool` if `qualified_name` was removed from the registry.
     */
    static bool unregister_class(std::string_view qualified_name);

    /**
     * Remove all classes loaded by a module.
     *
     * @param module_name Name of the module to remove.
     */
    static void unregister_module(std::string_view module_name);

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

/** Automatic property registration helper.  */
// FIXME This depends on static initialization order.
struct PropertyRegistration
{
    PropertyRegistration(
      const ClassInfo* (*class_provider)() noexcept,
      PropertyDescriptor& descriptor) noexcept
    {
        auto* cls = const_cast<ClassInfo*>(class_provider());
        cls->register_property(descriptor);
    }
};

#define DECLARE_CLASS_CORE(Type)                             \
    static constexpr std::string_view module_name = "Scene"; \
    static const ClassInfo* static_class() noexcept;         \
    static std::unique_ptr<Object> create_instance();        \
                                                             \
private:                                                     \
    static const AutoClassRegistrar auto_class_registrar;

#define DECLARE_ROOT_CLASS(Type) \
public:                          \
    DECLARE_CLASS_CORE(Type)

#define DECLARE_CLASS(Type, Base)               \
public:                                         \
    using Super = Base;                         \
    DECLARE_CLASS_CORE(Type)                    \
                                                \
public:                                         \
    const ClassInfo* get_class() const override \
    {                                           \
        return Type::static_class();            \
    }

#define DEFINE_CLASS_COMMON(Type, base_class_getter)    \
    namespace                                           \
    {                                                   \
    ClassInfo g_class_storage_##Type{};                 \
                                                        \
    static_assert(                                      \
      (base_class_getter) == nullptr                    \
        || std::is_invocable_v<                         \
          decltype(base_class_getter)>,                 \
      "base_class_getter must be invocable");           \
    PendingClassRegistration g_class_reg_##Type{        \
      .name = #Type,                                    \
      .module_name = Type::module_name,                 \
      .size = sizeof(Type),                             \
      .storage = &g_class_storage_##Type,               \
      .get_base_class = (base_class_getter),            \
      .factory = &Type::create_instance,                \
    };                                                  \
                                                        \
    PendingClassNode g_class_node_##Type{               \
      .reg = &g_class_reg_##Type,                       \
      .next = nullptr,                                  \
    };                                                  \
    }                                                   \
                                                        \
    const ClassInfo* Type::static_class() noexcept      \
    {                                                   \
        return &g_class_storage_##Type;                 \
    }                                                   \
                                                        \
    std::unique_ptr<Object> Type::create_instance()     \
    {                                                   \
        return std::make_unique<Type>();                \
    }                                                   \
                                                        \
    const AutoClassRegistrar Type::auto_class_registrar \
    {                                                   \
        &g_class_node_##Type                            \
    }

#define DEFINE_ROOT_CLASS(Type) \
    DEFINE_CLASS_COMMON(Type, nullptr)

#define DEFINE_CLASS(Type) \
    DEFINE_CLASS_COMMON(Type, &Type::Super::static_class)

#define REGISTER_PROPERTY_READONLY(Type, name, label)              \
    namespace                                                      \
    {                                                              \
    PropertyDescriptor Type##_##name{                              \
      #name,                                                       \
      label,                                                       \
      true,                                                        \
      &construct_member<Type, decltype(Type::name), &Type::name>}; \
    PropertyRegistration Type##_##name##_registration{             \
      &Type::static_class,                                         \
      Type##_##name};                                              \
    }

#define REGISTER_PROPERTY_READWRITE(Type, name, label)             \
    namespace                                                      \
    {                                                              \
    PropertyDescriptor Type##_##name{                              \
      #name,                                                       \
      label,                                                       \
      false,                                                       \
      &construct_member<Type, decltype(Type::name), &Type::name>}; \
    PropertyRegistration Type##_##name##_registration{             \
      &Type::static_class,                                         \
      Type##_##name};                                              \
    }

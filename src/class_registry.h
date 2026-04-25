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
    /** Module name of the class. */
    std::string_view module_name{};

    /** The class name. */
    std::string_view name{};

    /** Byte size of the class. */
    std::size_t size{0};

    /** Pointer to the static `ClassInfo` instance/storage. */
    ClassInfo* storage{nullptr};

    /** Return the base class. */
    ClassInfo* (*get_base_class)() noexcept = nullptr;

    /** Instance creation. */
    FactoryFn factory{nullptr};

    /** Property registration. */
    PropertyRegisterFn register_properties{nullptr};

    /** Constructor. */
    PendingClassRegistration() = delete;
    PendingClassRegistration(const PendingClassRegistration&) = delete;
    PendingClassRegistration(PendingClassRegistration&&) = delete;

    PendingClassRegistration& operator=(const PendingClassRegistration&) = delete;
    PendingClassRegistration& operator=(PendingClassRegistration&&) = delete;

    PendingClassRegistration(
      std::string_view module_name,
      std::string_view name,
      std::size_t size,
      ClassInfo* storage,
      ClassInfo* (*get_base_class)() noexcept,
      FactoryFn factory,
      PropertyRegisterFn register_properties) noexcept
    : module_name{module_name}
    , name{name}
    , size{size}
    , storage{storage}
    , get_base_class{get_base_class}
    , factory{factory}
    , register_properties{register_properties}
    {
    }
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
     */
    static void unregister_module(
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

#define DECLARE_CLASS_CORE(Module, Type)                     \
    using ThisClass = Type;                                  \
    static constexpr std::string_view module_name = #Module; \
    static ClassInfo* static_class() noexcept;               \
    static std::unique_ptr<Object> create_instance();        \
                                                             \
private:                                                     \
    static const AutoClassRegistrar auto_class_registrar;

#define DECLARE_ROOT_CLASS(Module, Type)               \
    std::vector<std::unique_ptr<Property>> properties; \
                                                       \
public:                                                \
    DECLARE_CLASS_CORE(Module, Type)

#define DECLARE_CLASS(Module, Type, Base)       \
public:                                         \
    using Super = Base;                         \
    DECLARE_CLASS_CORE(Module, Type)            \
                                                \
public:                                         \
    const ClassInfo* get_class() const override \
    {                                           \
        return Type::static_class();            \
    }

#define DEFINE_CLASS_COMMON(Type)                       \
    ClassInfo* Type::static_class() noexcept            \
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

#define DEFINE_ROOT_CLASS(Type)                  \
    namespace                                    \
    {                                            \
    ClassInfo g_class_storage_##Type{};          \
    PendingClassRegistration g_class_reg_##Type{ \
      Type::module_name,                         \
      #Type,                                     \
      sizeof(Type),                              \
      &g_class_storage_##Type,                   \
      nullptr,                                   \
      &Type::create_instance,                    \
      &Type::register_properties};               \
                                                 \
    PendingClassNode g_class_node_##Type{        \
      .reg = &g_class_reg_##Type,                \
      .next = nullptr,                           \
    };                                           \
    }                                            \
    DEFINE_CLASS_COMMON(Type)

#define DEFINE_CLASS(Type)                       \
    namespace                                    \
    {                                            \
    ClassInfo g_class_storage_##Type{};          \
    PendingClassRegistration g_class_reg_##Type{ \
      Type::module_name,                         \
      #Type,                                     \
      sizeof(Type),                              \
      &g_class_storage_##Type,                   \
      &Type::Super::static_class,                \
      &Type::create_instance,                    \
      &Type::register_properties};               \
                                                 \
    PendingClassNode g_class_node_##Type{        \
      .reg = &g_class_reg_##Type,                \
      .next = nullptr,                           \
    };                                           \
    }                                            \
    DEFINE_CLASS_COMMON(Type)

#define PROPERTY(Name, Label, Flags)                                                 \
    {                                                                                \
        auto property_##Name = std::make_unique<PropertyDescriptor>(                 \
          #Name,                                                                     \
          Label,                                                                     \
          Flags,                                                                     \
          &construct_member<ThisClass, decltype(ThisClass::Name), &ThisClass::Name>, \
          std::move(class_info.first_property));                                     \
        class_info.first_property = std::move(property_##Name);                      \
    }

/**
 * Software Rasterizer Playground.
 *
 * Reflection class registration.
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

struct PendingClassRegistration
{
    std::string_view name{};
    std::string_view module_name{};
    std::size_t size{0};

    ClassInfo* storage{nullptr};
    const ClassInfo* (*get_base_class)() noexcept = nullptr;
    FactoryFn factory{nullptr};
};

struct PendingClassNode
{
    const PendingClassRegistration* reg{nullptr};
    PendingClassNode* next{nullptr};
};

class ReflectionSystem
{
public:
    static void allow_auto_registration(bool enabled) noexcept;
    static bool is_auto_registration_allowed() noexcept;

    static void process_pending_registrations();
    static const ClassInfo* find_class(std::string_view qualified_name) noexcept;
    static void unregister_class(std::string_view qualified_name);
    static void unregister_module(std::string_view module_name);
    static void clear();

private:
    friend struct AutoClassRegistrar;
    static void add_pending(PendingClassNode* node) noexcept;
};

struct AutoClassRegistrar
{
    explicit AutoClassRegistrar(PendingClassNode* node) noexcept
    {
        ReflectionSystem::add_pending(node);
    }
};

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

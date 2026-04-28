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

#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <vector>

#include "class_info.h"

namespace reflect
{

namespace detail
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

    /** Resolver used to determine the super class during registration finalization. */
    ClassInfo::SuperResolverFn resolve_super{nullptr};

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

/** Automatic class registration helper. Executed during static initialization. */
struct AutoClassRegistrar;

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
    requires std::derived_from<T, Root>
             && std::default_initializable<T>
void* factory()
{
    return static_cast<Root*>(new T{});
}

/**
 * Destroy an instance of a child class of `Root`.
 *
 * @tparam Root Root type for the class hierarchy.
 * @param instance The instance to destroy.
 */
template<typename Root>
    requires std::has_virtual_destructor_v<Root>
void destroy(
  void* instance)
{
    delete static_cast<Root*>(instance);
}

/** Return a type's super class or `nullptr` if there is none. */
template<typename T>
const ClassInfo* resolve_super_class() noexcept
{
    return T::Super::static_class();
}

/** Return a type's super-class resolver or `nullptr` if there is none. */
template<typename T>
ClassInfo::SuperResolverFn super_class_resolver() noexcept
{
    if constexpr(requires { typename T::Super; })
    {
        return &resolve_super_class<T>;
    }

    return nullptr;
}

}    // namespace detail

/**
 * Reflection system.
 *
 * Thread safety:
 * - This API is not internally synchronized.
 * - Callers may provide external synchronization when using it from multiple
 *   threads (for example around concurrent DLL loading/registration).
 */
class ReflectionSystem
{
    friend struct detail::AutoClassRegistrar;

    /**
     * Add a class to the pending registration list.
     *
     * @param node The class info inside a class node.
     */
    static void add_pending(
      detail::PendingClassNode* node) noexcept;

public:
    /**
     * Enable/disable automatic queuing of class registrations.
     *
     * When enabled (default), static registrars append classes to the pending
     * list during static initialization. Call `process_pending_registrations()`
     * to finalize and publish them.
     *
     * To finalize, disable auto-registration first, then process pending entries.
     *
     * @param enabled Whether to enable automatic queuing.
     */
    static void allow_auto_registration(
      bool enabled) noexcept;

    /** Whether automatic registration is currently allowed. */
    static bool is_auto_registration_allowed() noexcept;

    /**
     * Process all queued registrations and publish them to the class registry.
     *
     * Call this only after disabling automatic queuing via
     * `allow_auto_registration(false)`, e.g. once dynamic library loading ended.
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
          detail::root_type_tag<Root>());
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
          detail::root_type_tag<Root>());
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

/** Automatic class registration helper. Queues entries during static initialization. */
struct detail::AutoClassRegistrar
{
    /**
     * Register a class node.
     *
     * @param node The node to add.
     */
    explicit AutoClassRegistrar(
      detail::PendingClassNode* node) noexcept
    {
        ReflectionSystem::add_pending(node);
    }
};

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
    requires requires {
        { TypeReflection<Root, T>::module_name } -> std::convertible_to<std::string_view>;
        { TypeReflection<Root, T>::class_name } -> std::convertible_to<std::string_view>;
        { TypeReflection<Root, T>::register_properties } -> std::convertible_to<ClassInfo::PropertyRegisterFn>;
    }
struct StaticClassRegistration
{
    ClassInfo storage{};
    detail::PendingClassRegistration registration{
      .module_name = TypeReflection<Root, T>::module_name,
      .name = TypeReflection<Root, T>::class_name,
      .size = sizeof(T),
      .storage = &storage,
      .resolve_super = detail::super_class_resolver<T>(),
      .root_tag = detail::root_type_tag<Root>(),
      .factory = &detail::factory<Root, T>,
      .destroy = &detail::destroy<Root>,
      .register_properties = TypeReflection<Root, T>::register_properties};
    detail::PendingClassNode node{
      .reg = &registration,
      .next = nullptr};
    detail::AutoClassRegistrar registrar{&node};
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
protected:
    /** RTTI-style type info. */
    const ClassInfo* class_info{Base::static_class()};

    /** Reflected properties, filled in by `initialize_properties`. */
    std::vector<std::unique_ptr<Property>> properties;

    /** Initialize the property list from the class metadata. */
    void initialize_properties()
    {
        properties.clear();

        std::vector<const reflect::ClassInfo*> class_chain;

        // Gather class chain so base class properties come first.
        for(const auto* cls = get_class(); cls != nullptr; cls = cls->get_super())
        {
            class_chain.push_back(cls);
        }

        for(const auto& cls: class_chain | std::views::reverse)
        {
            if(cls->first_property == nullptr)
            {
                continue;
            }

            for(auto* descriptor = cls->first_property.get();
                descriptor != nullptr;
                descriptor = descriptor->next ? descriptor->next.get() : nullptr)
            {
                if(descriptor->construct == nullptr)
                {
                    continue;
                }

                properties.emplace_back(
                  descriptor->construct(
                    this,
                    descriptor->name,
                    descriptor->label,
                    descriptor->flags,
                    descriptor->constraint));
            }
        }
    }

public:
    using Root = Base;

    /** Default property registration hook (no-op). */
    static void register_properties(
      [[maybe_unused]] ClassInfo& class_info)
    {
    }

    /**
     * Constructor.
     *
     * @note Creates an invalid object (no class info set).
     */
    ReflectRoot() = default;

    ReflectRoot(const ReflectRoot&) = delete;

    ReflectRoot(ReflectRoot&& other)
    : class_info{other.class_info}
    , properties{std::move(other.properties)}
    {
        if(class_info == nullptr)
        {
            throw std::runtime_error("Cannot create root without class_info.");
        }
    }

    /** Virtual destructor. */
    virtual ~ReflectRoot() = default;

    /**
     * Constructor.
     *
     * @param class_info The `ClassInfo` for this class.
     * @throws Throws a `std::runtime_error` if `class_info` is `nullptr`.
     */
    ReflectRoot(
      const ClassInfo* class_info)
    : class_info{class_info}
    {
        if(class_info == nullptr)
        {
            throw std::runtime_error("Cannot create root without class_info.");
        }

        initialize_properties();
    }

    ReflectRoot& operator=(const ReflectRoot&) = delete;

    ReflectRoot& operator=(ReflectRoot&& other)
    {
        class_info = other.class_info;
        properties = std::move(other.properties);

        other.class_info = nullptr;

        return *this;
    }

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

    /**
     * Returns the reflection metadata for this instance.
     *
     * This is the virtual, instance-level counterpart to `static_class()`,
     * allowing access to the concrete type's `ClassInfo` through a base
     * class pointer or reference.
     */
    virtual const ClassInfo* get_class() const
    {
        assert(class_info != nullptr);
        return class_info;
    }

    /**
     * Checks whether this instance is of type `T` or derived from `T`.
     *
     * @tparam T The class type to check. Must be part of the same
     *           reflection hierarchy.
     * @returns `true` if this instance's dynamic type is `T` or a subclass of `T`.
     */
    template<typename T>
        requires std::is_base_of_v<Root, T>
    bool is_a() const
    {
        return get_class()->is_a(T::static_class());
    }

    /**
     * Checks whether this instance is of the given class or derived from it.
     *
     * @param cls The class to check against.
     * @returns `true` if this instance's dynamic type is `cls` or a subclass of it.
     */
    bool is_a(const reflect::ClassInfo* cls) const
    {
        return get_class()->is_a(cls);
    }

    /** Return the properties for this class. */
    template<class Self>
    decltype(auto) get_properties(this Self& self)
    {
        return (self.ReflectRoot<Root>::properties);
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
 * Concept to ensure a class `Derived` has a `register_properties` function that is
 * distinct from its parent `Super`.
 */
template<
  class Derived,
  class Super>
concept HasOwnRegisterProperties =
  requires {
      { &Derived::register_properties }
        -> std::same_as<void (*)(ClassInfo&)>;
      { &Super::register_properties }
        -> std::same_as<void (*)(ClassInfo&)>;
  }
  && (&Derived::register_properties != &Super::register_properties);

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
        // Validate register_properties here, since it is evaluated
        // after instantiation (thus it cannot be a requires clause).
        static_assert(
          HasOwnRegisterProperties<Derived, Super>,
          "Each reflected class must declare its own "
          "static void register_properties(ClassInfo&).");

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
      &detail::construct_member_erased<MemberPtr>,
      std::move(class_info.first_property),
      nullptr,
      nullptr);
    class_info.first_property = std::move(descriptor);
}

template<auto MemberPtr, typename Constraint>
    requires std::is_base_of_v<
      PropertyConstraint,
      std::remove_cvref_t<Constraint>>
void register_property(
  ClassInfo& class_info,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  Constraint constraint)
{
    using MemberPtrTraits = MemberPointerTraits<decltype(MemberPtr)>;
    using MemberType = typename MemberPtrTraits::MemberType;
    using UnwrappedType = typename UnwrapType<MemberType>::ValueType;
    using ConstraintType = std::remove_cvref_t<Constraint>;
    static_assert(
      std::is_base_of_v<PropertyConstraint, ConstraintType>,
      "Constraint must derive from PropertyConstraint.");
    if constexpr(requires { typename ConstraintType::ValueType; })
    {
        using ConstraintValueType = typename ConstraintType::ValueType;
        static_assert(
          std::is_same_v<UnwrappedType, ConstraintValueType>,
          "Typed constraints with ValueType must match the reflected member type.");
    }

    auto descriptor = std::make_unique<PropertyDescriptor>(
      std::string{name},
      std::string{label},
      flags,
      &detail::construct_member_erased<MemberPtr>,
      std::move(class_info.first_property),
      std::make_shared<ConstraintType>(std::move(constraint)),
      nullptr);
    class_info.first_property = std::move(descriptor);
}

template<auto MemberPtr>
void register_property(
  ClassInfo& class_info,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  std::shared_ptr<const PropertyConstraint> constraint)
{
    auto descriptor = std::make_unique<PropertyDescriptor>(
      std::string{name},
      std::string{label},
      flags,
      &detail::construct_member_erased<MemberPtr>,
      std::move(class_info.first_property),
      std::move(constraint),
      nullptr);
    class_info.first_property = std::move(descriptor);
}

/**
 * Register a data member as a reflected property with a typed default value.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param class_info Class metadata to append the property to.
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 * @param default_value Typed default value for descriptor metadata.
 */
template<auto MemberPtr, typename DefaultValue>
    requires(!std::is_base_of_v<
             PropertyConstraint,
             std::remove_cvref_t<DefaultValue>>)
void register_property(
  ClassInfo& class_info,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  DefaultValue default_value)
{
    using MemberPtrTraits = MemberPointerTraits<decltype(MemberPtr)>;
    using MemberType = typename MemberPtrTraits::MemberType;
    using UnwrappedType = typename UnwrapType<MemberType>::ValueType;
    using DefaultType = std::remove_cvref_t<DefaultValue>;
    static_assert(
      std::is_same_v<UnwrappedType, DefaultType>,
      "Default value type must match the reflected member type.");

    auto descriptor = std::make_unique<PropertyDescriptor>(
      std::string{name},
      std::string{label},
      flags,
      &detail::construct_member_erased<MemberPtr>,
      std::move(class_info.first_property),
      nullptr,
      std::make_shared<TypedDefault<DefaultType>>(std::move(default_value)));
    class_info.first_property = std::move(descriptor);
}

/**
 * Register a data member as a reflected property with typed constraint and typed default metadata.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param class_info Class metadata to append the property to.
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 * @param constraint Constraint metadata for the property values.
 * @param default_value Typed default value for descriptor metadata.
 */
template<auto MemberPtr, typename Constraint, typename DefaultValue>
    requires std::is_base_of_v<
      PropertyConstraint,
      std::remove_cvref_t<Constraint>>
void register_property(
  ClassInfo& class_info,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  Constraint constraint,
  DefaultValue default_value)
{
    using MemberPtrTraits = MemberPointerTraits<decltype(MemberPtr)>;
    using MemberType = typename MemberPtrTraits::MemberType;
    using UnwrappedType = typename UnwrapType<MemberType>::ValueType;
    using ConstraintType = std::remove_cvref_t<Constraint>;
    using DefaultType = std::remove_cvref_t<DefaultValue>;
    static_assert(
      std::is_same_v<UnwrappedType, DefaultType>,
      "Default value type must match the reflected member type.");
    if constexpr(requires { typename ConstraintType::ValueType; })
    {
        using ConstraintValueType = typename ConstraintType::ValueType;
        static_assert(
          std::is_same_v<UnwrappedType, ConstraintValueType>,
          "Typed constraints with ValueType must match the reflected member type.");
    }

    auto descriptor = std::make_unique<PropertyDescriptor>(
      std::string{name},
      std::string{label},
      flags,
      &detail::construct_member_erased<MemberPtr>,
      std::move(class_info.first_property),
      std::make_shared<ConstraintType>(std::move(constraint)),
      std::make_shared<TypedDefault<DefaultType>>(std::move(default_value)));
    class_info.first_property = std::move(descriptor);
}

/**
 * Register a data member as a reflected property with shared constraint/default metadata.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param class_info Class metadata to append the property to.
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 * @param constraint Shared constraint metadata.
 * @param default_value Shared default value metadata.
 */
template<auto MemberPtr>
void register_property(
  ClassInfo& class_info,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  std::shared_ptr<const PropertyConstraint> constraint,
  std::shared_ptr<const PropertyDefault> default_value)
{
    auto descriptor = std::make_unique<PropertyDescriptor>(
      std::string{name},
      std::string{label},
      flags,
      &detail::construct_member_erased<MemberPtr>,
      std::move(class_info.first_property),
      std::move(constraint),
      std::move(default_value));
    class_info.first_property = std::move(descriptor);
}

/**
 * Register a data member as a reflected property with shared default metadata.
 *
 * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
 *
 * @param class_info Class metadata to append the property to.
 * @param name Internal property name.
 * @param label Display name / label (e.g. for UI/editor).
 * @param flags Static property flags.
 * @param default_value Shared default value metadata.
 */
template<auto MemberPtr>
void register_property(
  ClassInfo& class_info,
  std::string_view name,
  std::string_view label,
  PropertyFlags flags,
  std::shared_ptr<const PropertyDefault> default_value)
{
    auto descriptor = std::make_unique<PropertyDescriptor>(
      std::string{name},
      std::string{label},
      flags,
      &detail::construct_member_erased<MemberPtr>,
      std::move(class_info.first_property),
      nullptr,
      std::move(default_value));
    class_info.first_property = std::move(descriptor);
}

}    // namespace reflect

/**
 * Software Rasterizer Playground.
 *
 * Class metadata for object reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cstddef>
#include <string_view>

#include "containers/string.h"
#include "property.h"

namespace reflect
{

/**
 * Class info for RTTI-style object queries and editor metadata.
 *
 * Thread safety:
 * - This type does not provide internal synchronization.
 * - Callers must provide external synchronization when mutating/iterating concurrently.
 */
struct ClassInfo
{
    using FactoryFn = void* (*)();
    using DestroyFn = void (*)(void*);
    using PropertyRegisterFn = void (*)(ClassInfo&);
    using SuperResolverFn = const ClassInfo* (*)();

    /** Module name of the class. */
    swr::string module_name;

    /** The class name. */
    swr::string name;

    /** Qualified name as `module_name.name`. */
    swr::string qualified_name;

    /** Byte size of the class. */
    std::size_t size{0};

    /** Alignment of the class. */
    std::size_t alignment{0};

    /** Super-class info. */
    const ClassInfo* super{nullptr};

    /** Super-class resolver used during registration finalization. */
    SuperResolverFn resolve_super{nullptr};

    /** Root hierarchy marker for this class. */
    const void* root_tag{nullptr};

    /** Type-erased instance creation function. */
    FactoryFn factory{nullptr};

    /** Type-erased instance destruction function. */
    DestroyFn destroy{nullptr};

    /** Property registration. */
    PropertyRegisterFn register_properties{nullptr};

    /** Linked list of registered properties for this class. */
    swr::unique_ptr<PropertyDescriptor> first_property;

    /** Get this class' direct super class. Returns `nullptr` if there is none. */
    const ClassInfo* get_super() const
    {
        return super;
    }

    /*
     * Property registration.
     */

    /**
     * Register a data member as a reflected property.
     *
     * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     */
    template<auto MemberPtr>
    void register_property(
      std::string_view name,
      std::string_view label,
      PropertyFlags flags = PropertyFlags::None)
    {
        auto descriptor = swr::make_unique<
          PropertyDescriptor>(
          name,
          label,
          flags,
          &detail::construct_member_erased<MemberPtr>,
          std::move(first_property),
          nullptr,
          nullptr);
        first_property = std::move(descriptor);
    }

    /**
     * Register a data member as a reflected property.
     *
     * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
     * @tparam Contraint Constraint type.
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     * @param constraint Property constraint.
     */
    template<auto MemberPtr, typename Constraint>
        requires std::is_base_of_v<
          PropertyConstraint,
          std::remove_cvref_t<Constraint>>
    void register_property(
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

        auto descriptor = swr::make_unique<PropertyDescriptor>(
          name,
          label,
          flags,
          &detail::construct_member_erased<MemberPtr>,
          std::move(first_property),
          std::make_shared<ConstraintType>(std::move(constraint)),
          nullptr);
        first_property = std::move(descriptor);
    }

    /**
     * Register a data member as a reflected property.
     *
     * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     * @param constraint Property constraint.
     */
    template<auto MemberPtr>
    void register_property(
      std::string_view name,
      std::string_view label,
      PropertyFlags flags,
      swr::shared_ptr<const PropertyConstraint> constraint)
    {
        auto descriptor = swr::make_unique<PropertyDescriptor>(
          name,
          label,
          flags,
          &detail::construct_member_erased<MemberPtr>,
          std::move(first_property),
          std::move(constraint),
          nullptr);
        first_property = std::move(descriptor);
    }

    /**
     * Register a data member as a reflected property with a typed default value.
     *
     * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
     *
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

        auto descriptor = swr::make_unique<PropertyDescriptor>(
          name,
          label,
          flags,
          &detail::construct_member_erased<MemberPtr>,
          std::move(first_property),
          nullptr,
          std::make_shared<TypedDefault<DefaultType>>(std::move(default_value)));
        first_property = std::move(descriptor);
    }

    /**
     * Register a data member as a reflected property with typed constraint and typed default metadata.
     *
     * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
     *
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

        auto descriptor = swr::make_unique<PropertyDescriptor>(
          name,
          label,
          flags,
          &detail::construct_member_erased<MemberPtr>,
          std::move(first_property),
          std::make_shared<ConstraintType>(std::move(constraint)),
          std::make_shared<TypedDefault<DefaultType>>(std::move(default_value)));
        first_property = std::move(descriptor);
    }

    /**
     * Register a data member as a reflected property with shared constraint/default metadata.
     *
     * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     * @param constraint Shared constraint metadata.
     * @param default_value Shared default value metadata.
     */
    template<auto MemberPtr>
    void register_property(
      std::string_view name,
      std::string_view label,
      PropertyFlags flags,
      swr::shared_ptr<const PropertyConstraint> constraint,
      swr::shared_ptr<const PropertyDefault> default_value)
    {
        auto descriptor = swr::make_unique<PropertyDescriptor>(
          name,
          label,
          flags,
          &detail::construct_member_erased<MemberPtr>,
          std::move(first_property),
          std::move(constraint),
          std::move(default_value));
        first_property = std::move(descriptor);
    }

    /**
     * Register a data member as a reflected property with shared default metadata.
     *
     * @tparam MemberPtr Pointer to member (e.g. `&Class::member`).
     *
     * @param name Internal property name.
     * @param label Display name / label (e.g. for UI/editor).
     * @param flags Static property flags.
     * @param default_value Shared default value metadata.
     */
    template<auto MemberPtr>
    void register_property(
      std::string_view name,
      std::string_view label,
      PropertyFlags flags,
      swr::shared_ptr<const PropertyDefault> default_value)
    {
        auto descriptor = swr::make_unique<PropertyDescriptor>(
          name,
          label,
          flags,
          &detail::construct_member_erased<MemberPtr>,
          std::move(first_property),
          nullptr,
          std::move(default_value));
        first_property = std::move(descriptor);
    }

    /**
     * Find a registered property descriptor by internal property name.
     *
     * @param property_name Internal property name (`DescriptorBase::name`).
     * @returns Returns the first matching descriptor, or `nullptr` if not found.
     */
    PropertyDescriptor* find_property(std::string_view property_name) noexcept
    {
        for(auto* descriptor = first_property.get();
            descriptor != nullptr;
            descriptor = descriptor->next.get())
        {
            if(descriptor->name == property_name)
            {
                return descriptor;
            }
        }

        return nullptr;
    }

    /**
     * Find a registered property descriptor by internal property name.
     *
     * Searches this class first, then walks through superclasses.
     *
     * @param property_name Internal property name (`PropertyDescriptor::name`).
     * @returns The first matching descriptor, or `nullptr` if not found.
     */
    const PropertyDescriptor* find_property(std::string_view property_name) const
    {
        for(const auto* cls = this; cls != nullptr; cls = cls->get_super())
        {
            for(const auto* descriptor = cls->first_property.get();
                descriptor != nullptr;
                descriptor = descriptor->next.get())
            {
                if(descriptor->name == property_name)
                {
                    return descriptor;
                }
            }
        }

        return nullptr;
    }

    /**
     * Check if this class is a child of another class.
     *
     * @param other The potential super class.
     * @returns Returns `true` if `other` is a super class.
     */
    bool is_a(const ClassInfo* other) const
    {
        for(auto p = this; p != nullptr; p = p->get_super())
        {
            if(p == other)
            {
                return true;
            }
        }
        return false;
    }
};

}    // namespace reflect

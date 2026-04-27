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
#include <string>
#include <string_view>

#include "property.h"

namespace reflect
{

/** Class info for RTTI-style object queries and editor metadata. */
struct ClassInfo
{
    using FactoryFn = void* (*)();
    using DestroyFn = void (*)(void*);
    using PropertyRegisterFn = void (*)(ClassInfo&);

    /** Module name of the class. */
    std::string module_name;

    /** The class name. */
    std::string name;

    /** Qualified name as `module_name.name`. */
    std::string qualified_name;

    /** Byte size of the class. */
    std::size_t size{0};

    /** Super-class info. */
    const ClassInfo* super{nullptr};

    /** Root hierarchy marker for this class. */
    const void* root_tag{nullptr};

    /** Type-erased instance creation function. */
    FactoryFn factory{nullptr};

    /** Type-erased instance destruction function. */
    DestroyFn destroy{nullptr};

    /** Property registration. */
    PropertyRegisterFn register_properties{nullptr};

    /** Linked list of registered properties for this class. */
    std::unique_ptr<PropertyDescriptor> first_property;

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
     * @param property_name Internal property name (`DescriptorBase::name`).
     * @returns Returns the first matching descriptor, or `nullptr` if not found.
     */
    const PropertyDescriptor* find_property(std::string_view property_name) const noexcept
    {
        for(const auto* descriptor = first_property.get();
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
     * Check if this class is a child of another class.
     *
     * @param other The potential super class.
     * @returns Returns `true` if `other` is a super class.
     */
    bool is_a(const ClassInfo* other) const
    {
        for(auto p = this; p != nullptr; p = p->super)
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

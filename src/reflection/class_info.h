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
#include <stdexcept>

#include "property.h"

class Object;

namespace reflect
{

struct ClassInfo;

using FactoryFn = std::unique_ptr<Object> (*)();
using PropertyRegisterFn = void (*)(ClassInfo&);

/** Class info for RTTI-style object queries and editor metadata. */
struct ClassInfo
{
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

    /** Instance creation. */
    FactoryFn factory{nullptr};

    /** Property registration. */
    PropertyRegisterFn register_properties{nullptr};

    /** Linked list of registered properties for this class. */
    std::unique_ptr<PropertyDescriptor<void>> first_property;

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

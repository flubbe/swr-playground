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

#include "property.h"

class Object;
struct ClassInfo;

using FactoryFn = std::unique_ptr<Object> (*)();

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

    /** Parent class info. */
    const ClassInfo* parent{nullptr};

    /** Instance creation. */
    FactoryFn factory{nullptr};

    /** Linked list of registered properties for this class. */
    PropertyDescriptor* first_property{nullptr};

    /**
     * Check if this class is a child of another class.
     *
     * @param other The potential base class.
     * @returns Returns `true` if `other` is a base class.
     */
    bool is_a(const ClassInfo* other) const
    {
        for(auto p = this; p != nullptr; p = p->parent)
        {
            if(p == other)
            {
                return true;
            }
        }
        return false;
    }
};

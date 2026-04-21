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
#include <memory>
#include <string>

#include "property.h"

class Object;

using FactoryFn = std::unique_ptr<Object> (*)();

/** Class info for RTTI-style object queries and editor metadata. */
struct ClassInfo
{
    std::string module_name;
    std::string name;
    std::string qualified_name;
    const ClassInfo* parent = nullptr;
    std::size_t size = 0;
    FactoryFn factory = nullptr;
    PropertyDescriptor* first_property = nullptr;

    void register_property(PropertyDescriptor& property) noexcept
    {
        property.next = first_property;
        first_property = &property;
    }

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

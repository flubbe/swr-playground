/**
 * Software Rasterizer Playground.
 *
 * Object implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <ranges>

#include "object.h"

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
DEFINE_ROOT_CLASS(Object);

void Object::register_properties(ClassInfo& class_info)
{
    PROPERTY(object_id, "Object ID", PropertyFlags::ReadOnly);
    PROPERTY(name, "Name", PropertyFlags::None);
    PROPERTY(transform, "Transform", PropertyFlags::None);
}

void Object::initialize_properties()
{
    properties.clear();

    std::vector<const ClassInfo*> class_chain;

    // Gather class chain so base class properties come first.
    for(const ClassInfo* cls = get_class(); cls != nullptr; cls = cls->parent)
    {
        class_chain.push_back(cls);
    }

    for(const auto& cls: class_chain | std::views::reverse)
    {
        if(cls->first_property == nullptr)
        {
            continue;
        }

        for(PropertyDescriptor* descriptor = cls->first_property.get();
            descriptor != nullptr;
            descriptor = descriptor->next ? descriptor->next.get() : nullptr)
        {
            if(descriptor->construct == nullptr)
            {
                continue;
            }

            properties.emplace_back(
              descriptor->construct(
                *this,
                descriptor->name,
                descriptor->label,
                descriptor->is_read_only()));
        }
    }
}

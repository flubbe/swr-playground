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

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Object);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void Object::register_properties(reflect::ClassInfo& class_info)
{
    register_property<&Object::object_id>(
      class_info,
      "object_id",
      "Object ID",
      reflect::PropertyFlags::ReadOnly);
    register_property<&Object::name>(
      class_info,
      "name",
      "Name",
      reflect::PropertyFlags::None);
    register_property<&Object::transform>(
      class_info,
      "transform",
      "Transform",
      reflect::PropertyFlags::None);
}

void Object::initialize_properties()
{
    properties.clear();

    std::vector<const reflect::ClassInfo*> class_chain;

    // Gather class chain so base class properties come first.
    for(const auto* cls = get_class(); cls != nullptr; cls = cls->super)
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
                *this,
                descriptor->name,
                descriptor->label,
                descriptor->flags));
        }
    }
}

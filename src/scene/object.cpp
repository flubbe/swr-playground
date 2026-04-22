/**
 * Software Rasterizer Playground.
 *
 * Object implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "object.h"

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
DEFINE_ROOT_CLASS(Object);

PropertyList Object::get_properties()
{
    std::vector<const ClassInfo*> class_chain;
    PropertyList properties;

    // Gather class chain so base class properties come first.
    for(const ClassInfo* cls = get_class(); cls != nullptr; cls = cls->parent)
    {
        class_chain.push_back(cls);
    }

    for(auto it = class_chain.rbegin(); it != class_chain.rend(); ++it)
    {
        const ClassInfo* cls = *it;

        for(PropertyDescriptor* descriptor = cls->first_property;
            descriptor != nullptr;
            descriptor = descriptor->next)
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
                descriptor->read_only));
        }
    }

    return properties;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
REGISTER_PROPERTY_READONLY(
  Object,
  object_id,
  "Object ID");

REGISTER_PROPERTY_READWRITE(
  Object,
  name,
  "Name");

REGISTER_PROPERTY_READWRITE(
  Object,
  transform,
  "Transform");

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

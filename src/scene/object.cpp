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

void Object::initialize_properties()
{
    std::vector<const ClassInfo*> class_chain;

    // gather class chain for property lookup.
    for(const ClassInfo* cls = get_class(); cls != nullptr; cls = cls->parent)
    {
        class_chain.push_back(cls);
    }

    // keep declaration order.
    for(auto it = class_chain.rbegin(); it != class_chain.rend(); ++it)
    {
        for(PropertyDescriptor* descriptor = (*it)->first_property;
            descriptor != nullptr;
            descriptor = descriptor->next)
        {
            properties.emplace_back(
              descriptor->construct(
                *this,
                descriptor->name,
                descriptor->label,
                descriptor->read_only));
        }
    }
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

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

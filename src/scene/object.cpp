/**
 * Software Rasterizer Playground.
 *
 * Class metadata for scene reflection.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "object.h"

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
DEFINE_ROOT_CLASS(Object);

Object::PropertyList Object::get_properties()
{
    PropertyList properties;

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
            switch(descriptor->field_type)
            {
            case PropertyDescriptor::FieldType::Int:
                if(descriptor->get_int == nullptr)
                {
                    break;
                }
                properties.emplace_back(
                  std::make_unique<IntProperty>(
                    std::string{descriptor->label},
                    descriptor->get_int(*this),
                    descriptor->read_only));
                break;
            case PropertyDescriptor::FieldType::UInt:
                if(descriptor->get_uint == nullptr)
                {
                    break;
                }
                properties.emplace_back(
                  std::make_unique<UIntProperty>(
                    std::string{descriptor->label},
                    descriptor->get_uint(*this),
                    descriptor->read_only));
                break;
            case PropertyDescriptor::FieldType::Float:
                if(descriptor->get_float == nullptr)
                {
                    break;
                }
                properties.emplace_back(
                  std::make_unique<FloatProperty>(
                    std::string{descriptor->label},
                    descriptor->get_float(*this),
                    descriptor->read_only));
                break;
            case PropertyDescriptor::FieldType::Bool:
                if(descriptor->get_bool == nullptr)
                {
                    break;
                }
                properties.emplace_back(
                  std::make_unique<BoolProperty>(
                    std::string{descriptor->label},
                    descriptor->get_bool(*this),
                    descriptor->read_only));
                break;
            case PropertyDescriptor::FieldType::String:
                if(descriptor->get_string == nullptr)
                {
                    break;
                }
                properties.emplace_back(
                  std::make_unique<StringProperty>(
                    std::string{descriptor->label},
                    descriptor->get_string(*this),
                    descriptor->read_only));
                break;
            }
        }
    }

    return properties;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

REGISTER_PROPERTY(
  Object,
  object_id,
  {
    .label = "Object ID",
    .field_type = PropertyDescriptor::FieldType::UInt,
    .get_uint = &Object::property_access_object_id_value,
    .read_only = true,
    .next = nullptr,
  });

REGISTER_PROPERTY(
  Object,
  name,
  {
    .label = "Name",
    .field_type = PropertyDescriptor::FieldType::String,
    .get_string = &Object::property_access_name,
    .read_only = false,
    .next = nullptr,
  });

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

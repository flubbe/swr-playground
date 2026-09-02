
/**
 * Software Rasterizer Playground.
 *
 * JSON property writer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "reflection/builtin_properties.h"
#include "scene/properties.h"
#include "serialization/except.h"
#include "property_writer.h"

namespace serial::json
{

JsonPropertyWriter::JsonPropertyWriter(
  JsonWriter& writer)
: writer{writer}
{
}

void JsonPropertyWriter::write_value(
  const reflect::Property& property,
  const void* storage)
{
    if(auto* p = property.try_as<reflect::IntProperty>())
    {
        writer.write_val(p->get_value(storage));
    }
    else if(auto* p = property.try_as<reflect::UIntProperty>())
    {
        writer.write_val(p->get_value(storage));
    }
    else if(auto* p = property.try_as<reflect::FloatProperty>())
    {
        writer.write_val(p->get_value(storage));
    }
    else if(auto* p = property.try_as<reflect::BoolProperty>())
    {
        writer.write_val(p->get_value(storage));
    }
    else if(auto* p = property.try_as<reflect::StringProperty>())
    {
        writer.write_val(p->get_value(storage));
    }
    else if(auto* p = property.try_as<reflect::PathProperty>())
    {
        writer.write_val(p->get_value(storage).string());
    }
    else if(auto* p = property.try_as<reflect::VectorProperty>())
    {
        writer.begin_array();

        for(std::size_t i = 0; i < p->get_element_count(storage); ++i)
        {
            write_value(
              p->get_inner(),
              p->get_element(storage, i));
        }

        writer.end_array();
    }
#if SWR_CUSTOM_STRING_TYPE
    else if(auto* p = property.try_as<reflect::SwrStringProperty>())
    {
        writer.write_val(p->get_value(storage));
    }
#endif /* SWR_CUSTOM_STRING_TYPE */
    else if(auto* p = property.try_as<reflect::Mat4Property>())
    {
        writer.begin_array();

        for(const auto& row: p->get_value(storage).rows)
        {
            writer.begin_array();
            writer.write_val(row.x);
            writer.write_val(row.y);
            writer.write_val(row.z);
            writer.write_val(row.w);
            writer.end_array();
        }

        writer.end_array();
    }
    else if(auto* p = property.try_as<reflect::Vec4Property>())
    {
        const auto& v = p->get_value(storage);

        writer.begin_array();
        writer.write_val(v.x);
        writer.write_val(v.y);
        writer.write_val(v.z);
        writer.write_val(v.w);
        writer.end_array();
    }
    else
    {
        throw SerializationError{
          std::format(
            "Unable to serialize property '{}' of unsupported type to JSON.",
            property.get_name())};
    }
}

void JsonPropertyWriter::visit(
  const reflect::Property& property,
  const void* storage)
{
    writer.write_key(property.get_name());
    write_value(property, storage);
}

}    // namespace serial::json


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
#include "json_property_writer.h"

namespace serial::json
{

JsonPropertyWriter::JsonPropertyWriter(
  JsonWriter& writer)
: writer{writer}
{
}

void JsonPropertyWriter::visit(
  reflect::Property& property)
{
    if(auto* p = property.try_as<reflect::IntProperty>())
    {
        writer.write_key_value(p->get_name(), p->get_value());
    }
    else if(auto* p = property.try_as<reflect::UIntProperty>())
    {
        writer.write_key_value(p->get_name(), p->get_value());
    }
    else if(auto* p = property.try_as<reflect::FloatProperty>())
    {
        writer.write_key_value(p->get_name(), p->get_value());
    }
    else if(auto* p = property.try_as<reflect::BoolProperty>())
    {
        writer.write_key_value(p->get_name(), p->get_value());
    }
    else if(auto* p = property.try_as<reflect::StringProperty>())
    {
        writer.write_key_value(p->get_name(), p->get_value());
    }
#if SWR_CUSTOM_STRING_TYPE
    else if(auto* p = property.try_as<reflect::SwrStringProperty>())
    {
        writer.write_key_value(p->get_name(), p->get_value());
    }
#endif /* SWR_CUSTOM_STRING_TYPE */
    else if(auto* p = property.try_as<reflect::Mat4Property>())
    {
        writer.write_key(p->get_name());

        writer.begin_array();

        for(const auto& row: p->get_value().rows)
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
        const auto& v = p->get_value();

        writer.write_key(p->get_name());

        writer.begin_array();
        writer.write_val(v.x);
        writer.write_val(v.y);
        writer.write_val(v.z);
        writer.write_val(v.w);
        writer.end_array();
    }
    else
    {
        // TODO Error, throw?
    }
}

}    // namespace serial::json

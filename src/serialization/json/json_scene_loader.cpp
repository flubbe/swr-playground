/**
 * Software Rasterizer Playground.
 *
 * JSON scene loader.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <simdjson.h>

#include "reflection/builtin_properties.h"
#include "reflection/construct.h"
#include "scene/properties.h"
#include "scene/scene.h"
#include "serialization/except.h"
#include "json_scene_loader.h"
#include "logging.h"

namespace
{

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"JSON"};
    return logger;
}

/** Load a value into a property. */
class DeserializerVisitor final
: public reflect::PropertyVisitor
{
    /** The object. */
    Object& object;

    /** Value to load. */
    simdjson::ondemand::value& value;

public:
    /**
     * Constructor.
     *
     * @param object The object to load the value into.
     * @param value The JSON value to load.
     */
    explicit DeserializerVisitor(
      Object& object,
      simdjson::ondemand::value& value)
    : object{object}
    , value{value}
    {
    }

    void visit(
      reflect::Property& property) override
    {
        if(auto* p = property.try_as<reflect::IntProperty>())
        {
            std::int64_t val{0};
            if(auto err = value.get_int64().get(val); !err)
            {
                p->set_value(static_cast<reflect::IntProperty::Type>(val));
            }
            else
            {
                get_logger().warningf(
                  "Unable to deserialize int64 '{}.{}' from JSON",
                  object.get_name(), p->get_name());
            }
        }
        else if(auto* p = property.try_as<reflect::UIntProperty>())
        {
            std::uint64_t val{0};
            if(auto err = value.get_uint64().get(val); !err)
            {
                p->set_value(static_cast<reflect::UIntProperty::Type>(val));
            }
            else
            {
                get_logger().warningf(
                  "Unable to deserialize uint64 '{}.{}' from JSON",
                  object.get_name(), p->get_name());
            }
        }
        else if(auto* p = property.try_as<reflect::FloatProperty>())
        {
            double val{0.0};
            if(auto err = value.get_double().get(val); !err)
            {
                p->set_value(static_cast<reflect::FloatProperty::Type>(val));
            }
            else
            {
                get_logger().warningf(
                  "Unable to deserialize float '{}.{}' from JSON",
                  object.get_name(), p->get_name());
            }
        }
        else if(auto* p = property.try_as<reflect::BoolProperty>())
        {
            bool val{false};
            if(auto err = value.get_bool().get(val); !err)
            {
                p->set_value(val);
            }
            else
            {
                get_logger().warningf(
                  "Unable to deserialize bool '{}.{}' from JSON",
                  object.get_name(), p->get_name());
            }
        }
        else if(auto* p = property.try_as<reflect::StringProperty>())
        {
            std::string val;
            if(auto err = value.get_string().get(val); !err)
            {
                p->set_value(val);
            }
            else
            {
                get_logger().warningf(
                  "Unable to deserialize string '{}.{}' from JSON",
                  object.get_name(), p->get_name());
            }
        }
#if SWR_CUSTOM_STRING_TYPE
        else if(auto* p = property.try_as<reflect::SwrStringProperty>())
        {
            std::string val;
            if(auto err = value.get_string().get(val); !err)
            {
                p->set_value(val);
            }
            else
            {
                get_logger().warningf(
                  "Unable to deserialize string '{}.{}' from JSON",
                  object.get_name(), p->get_name());
            }
        }
#endif /* SWR_CUSTOM_STRING_TYPE */
        else if(auto* p = property.try_as<reflect::Mat4Property>())
        {
            simdjson::ondemand::array outer_arr;
            if(auto err = value.get_array().get(outer_arr); !err)
            {
                ml::mat4x4 m;
                std::size_t row_idx = 0;

                for(auto row_elem: outer_arr)
                {
                    if(row_idx >= 4)
                    {
                        get_logger().warningf(
                          "Too many rows when deserializing mat4x4 '{}.{}' from JSON",
                          object.get_name(), p->get_name());

                        break;
                    }

                    simdjson::ondemand::array inner_arr;
                    if(auto inner_err = row_elem.get_array().get(inner_arr); !inner_err)
                    {
                        std::size_t col_idx = 0;
                        for(auto col_elem: inner_arr)
                        {
                            double d{};
                            if(!col_elem.get_double().get(d))
                            {
                                if(col_idx < 4)
                                {
                                    m.rows[row_idx][col_idx] = static_cast<float>(d);
                                }
                                else
                                {
                                    get_logger().warningf(
                                      "Too many columns when deserializing mat4x4 '{}.{}' from JSON",
                                      object.get_name(), p->get_name());

                                    break;
                                }
                            }
                            ++col_idx;
                        }

                        if(col_idx != 4)
                        {
                            get_logger().warningf(
                              "Too few columns when deserializing mat4x4 '{}.{}' from JSON",
                              object.get_name(), p->get_name());
                        }
                    }
                    ++row_idx;
                }

                if(row_idx != 4)
                {
                    get_logger().warningf(
                      "Too few rows when deserializing mat4x4 '{}.{}' from JSON",
                      object.get_name(), p->get_name());
                }

                p->set_value(m);
            }
            else
            {
                get_logger().warningf(
                  "Unable to deserialize mat4 '{}.{}' from JSON",
                  object.get_name(), p->get_name());
            }
        }
        else if(auto* p = property.try_as<reflect::Vec4Property>())
        {
            // Deserialize JSON array [x, y, z, w]
            simdjson::ondemand::array arr;
            if(auto err = value.get_array().get(arr); !err)
            {
                ml::vec4 v{};
                std::size_t idx = 0;
                for(auto elem: arr)
                {
                    double d{};
                    if(!elem.get_double().get(d))
                    {
                        v[idx] = static_cast<float>(d);
                    }
                    ++idx;
                }
                p->set_value(v);
            }
            else
            {
                get_logger().warningf(
                  "Unable to deserialize vec4 '{}.{}' from JSON",
                  object.get_name(), p->get_name());
            }
        }
        else
        {
            throw serial::SerializationError{
              std::format(
                "Unsupported property type for property '{}'",
                property.get_name())};
        }
    }
};

/** Helper for out-of-order processing. */
struct FieldEntry
{
    /** Key. */
    std::string_view key;

    /** Value. */
    simdjson::ondemand::value value;
};

/**
 * Load fields into the object.
 *
 * Traverses the fields, and writes each one found in the object.
 *
 * @param object The object to load the values into.
 * @param fields The fields to load.
 */
void deserialize_properties(
  Object& object,
  swr::vector<FieldEntry>& fields)
{
    for(auto& entry: fields)
    {
        auto prop_it = std::ranges::find_if(
          object.get_properties(),
          [key = entry.key](const auto& p)
          { return p->get_name() == key; });

        if(prop_it != object.get_properties().end())
        {
            DeserializerVisitor visitor{object, entry.value};
            (*prop_it)->accept(visitor);
        }
        else
        {
            get_logger().warningf(
              "Unknown field '{}' found for class '{}'",
              entry.key, object.get_class()->qualified_name);
        }
    }
}

/**
 * Construct objects and load their properties.
 *
 * @param scene The scene to add the objects to.
 * @param arr JSON array holding the objects.
 * @throws Throws a `SerializationError` if the `class` key is not found
 *     or if object construction failed.
 */
void load_objects(
  Scene& scene,
  simdjson::ondemand::array arr)
{
    for(auto item: arr)
    {
        simdjson::ondemand::object obj = item.get_object();

        swr::vector<FieldEntry> fields;
        std::string_view qualified_class_name;

        // Extract "class" and buffer other property entries
        for(auto field: obj)
        {
            std::string_view key = field.unescaped_key();

            simdjson::ondemand::value val;
            if(auto err = field.value().get(val); err)
            {
                get_logger().warningf(
                  "Failed to read JSON value for key '{}': {}",
                  key, simdjson::error_message(err));
                continue;
            }

            if(key == "class")
            {
                if(auto err = val.get_string().get(qualified_class_name); err)
                {
                    throw serial::SerializationError{
                      "Object entry missing required string field 'class'."};
                }
            }
            else
            {
                fields.push_back({key, val});
            }
        }

        if(qualified_class_name.empty())
        {
            throw serial::SerializationError{
              "Object entry missing required field 'class'."};
        }

        // Construct via reflection
        auto new_object = reflect::construct<Object>(qualified_class_name);
        if(!new_object)
        {
            throw serial::SerializationError{
              std::format(
                "Object construction failed for class '{}'.",
                qualified_class_name)};
        }

        // Deserialize properties using visitor
        deserialize_properties(*new_object, fields);

        scene.add_object(std::move(new_object));
    }
}

}    // namespace

namespace serial::json
{

void JsonSceneLoader::load(
  Scene& scene,
  std::string_view source_text)
{
    scene.clear();

    simdjson::padded_string padded_json(source_text);
    simdjson::ondemand::parser json_parser;

    simdjson::ondemand::document doc;
    if(auto err = json_parser.iterate(padded_json).get(doc); err)
    {
        throw std::runtime_error{
          std::format("JSON parsing failed: {}", simdjson::error_message(err))};
    }

    simdjson::ondemand::object root_obj;
    if(auto err = doc.get_object().get(root_obj); err)
    {
        throw std::runtime_error{"Root JSON value must be an object."};
    }

    for(auto field: root_obj)
    {
        std::string_view key = field.unescaped_key();

        if(key == "time")
        {
            scene.set_time(field.value().get_double());
        }
        else if(key == "paused")
        {
            scene.set_paused(field.value().get_bool());
        }
        else if(key == "objects")
        {
            load_objects(scene, field.value().get_array());
        }
        else
        {
            get_logger().warningf(
              "Unknown key '{}' found in scene.",
              key);
        }
    }
}

}    // namespace serial::json

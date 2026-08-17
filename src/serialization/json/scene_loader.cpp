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

#include "assets/resolver.h"
#include "reflection/builtin_properties.h"
#include "reflection/construct.h"
#include "scene/properties.h"
#include "scene/scene.h"
#include "serialization/except.h"
#include "property_deserializer.h"
#include "scene_loader.h"
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
            serial::json::JsonPropertyDeserializer visitor{
              get_logger(),
              object,
              entry.value};
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
    /*
     * Clear scene.
     */

    scene.clear();

    /*
     * Set up JSON processing.
     */

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

    /*
     * Load JSON into scene.
     */

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

    /*
     * Dependency resolution.
     */

    assets::Resolver resolver;
    scene.for_each_object<Object>(
      [&resolver](Object& obj)
      { obj.resolve(resolver); });

    /*
     * Post-load processing.
     */

    scene.for_each_object<Object>(
      [](Object& obj)
      { obj.post_load(); });
}

}    // namespace serial::json

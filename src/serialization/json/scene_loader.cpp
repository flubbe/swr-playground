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

#include "containers/unordered_set.h"
#include "reflection/builtin_properties.h"
#include "reflection/construct.h"
#include "scene/properties.h"
#include "scene/scene.h"
#include "serialization/except.h"
#include "property_deserializer.h"
#include "asset_resolver.h"
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

/**
 * Load fields into the object.
 *
 * Traverses the fields, and writes each one found in the object.
 *
 * @param object The object to load the values into.
 * @param obj The object to load.
 */
void deserialize_properties(
  Object& object,
  simdjson::dom::object json_obj)
{
    for(auto& property: object.get_properties())
    {
        auto value = json_obj[property->get_name()];
        if(value.error() == simdjson::NO_SUCH_FIELD)
        {
            get_logger().warningf(
              "Cannot deserialize missing property '{}' for class '{}'.",
              property->get_name(),
              object.get_class()->qualified_name);
            continue;
        }

        serial::json::JsonPropertyDeserializer visitor{
          get_logger(),
          object,
          value.value()};

        void* property_address = reinterpret_cast<std::byte*>(&object)
                                 + property->get_offset();
        property->accept(
          visitor,
          property_address);
    }

    const auto is_property = [&object](std::string_view name)
    {
        return std::ranges::any_of(
          object.get_properties(),
          [name](const auto& property)
          {
              return property->get_name() == name;
          });
    };

    for(auto& entry: json_obj)
    {
        if(entry.key != "class"
           && !is_property(entry.key))
        {
            get_logger().warningf(
              "Unknown field '{}' found for class '{}'.",
              entry.key,
              object.get_class()->qualified_name);
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
  simdjson::dom::array arr)
{
    for(auto item: arr)
    {
        simdjson::dom::object obj;
        if(auto err = item.get_object().get(obj))
        {
            throw serial::SerializationError{
              std::format(
                "Expected JSON object: {}",
                simdjson::error_message(err))};
        }

        auto class_value = obj["class"];
        if(class_value.error() == simdjson::NO_SUCH_FIELD)
        {
            throw serial::SerializationError{
              "Object entry missing required field 'class'."};
        }
        else if(auto err = class_value.error())
        {
            throw serial::SerializationError{
              std::format(
                "Failed to access 'class': {}",
                simdjson::error_message(err))};
        }

        auto qualified_class_name = class_value.get_string();
        if(qualified_class_name.error())
        {
            throw serial::SerializationError{
              std::format(
                "Entry 'class' name is not a string: {}",
                simdjson::error_message(qualified_class_name.error()))};
        }

        // Construct via reflection
        auto new_object = reflect::construct<Object>(qualified_class_name.value());
        if(!new_object)
        {
            throw serial::SerializationError{
              std::format(
                "Object construction failed for class '{}'.",
                qualified_class_name.value())};
        }

        // Deserialize properties using visitor
        deserialize_properties(*new_object, obj);

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

    simdjson::dom::parser json_parser;

    auto doc = json_parser.parse(source_text);
    if(doc.error())
    {
        throw std::runtime_error{
          std::format(
            "JSON parsing failed: {}",
            simdjson::error_message(doc.error()))};
    }

    auto json_object = doc.get_object();
    if(json_object.error())
    {
        throw std::runtime_error{"Root JSON value must be an object."};
    }

    /*
     * Load JSON into scene.
     */

    for(auto field: json_object)
    {
        std::string_view key = field.key;

        if(key == "time")
        {
            scene.set_time(field.value.get_double());
        }
        else if(key == "paused")
        {
            scene.set_paused(field.value.get_bool());
        }
        else if(key == "objects")
        {
            load_objects(scene, field.value.get_array());
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

    scene.for_each_object<Object>(
      [this](Object& obj)
      { obj.resolve(resolver); });

    /*
     * Post-load processing.
     */

    scene.for_each_object<Object>(
      [](Object& obj)
      { obj.post_load(); });
}

}    // namespace serial::json

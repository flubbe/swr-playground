/**
 * Software Rasterizer Playground.
 *
 * JSON scene loader.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <limits>

#include <simdjson.h>

#include "containers/unordered_set.h"
#include "reflection/builtin_properties.h"
#include "reflection/construct.h"
#include "scene/systems/animation.h"
#include "scene/systems/lights.h"
#include "scene/systems/object_tick.h"
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

/**
 * Construct the systems listed in the JSON array.
 *
 * @param scene The scene to add the systems to.
 * @param arr JSON array holding the system names.
 */
void load_systems(
  Scene& scene,
  simdjson::dom::array arr)
{
    for(auto item: arr)
    {
        auto entry = item.get_string();
        if(entry.error())
        {
            throw serial::SerializationError{
              std::format(
                "System array entry is not a string: {}",
                simdjson::error_message(entry.error()))};
        }

        // TODO Hard-coded system names for now.
        auto name = entry.value();
        if(name == "Animation")
        {
            scene.add_system<AnimationSystem>();
        }
        else if(name == "Light")
        {
            scene.add_system<LightSystem>();
        }
        else if(name == "Tick")
        {
            scene.add_system<ObjectTickSystem>();
        }
        else
        {
            get_logger().errorf(
              "Scene contains unknown system: {}",
              name);
        }
    }
}

/**
 * Load the spin animations listed in the JSON array.
 *
 * @param scene The scene to add the spin animations to.
 * @param arr JSON array holding the system names.
 */
void load_spin_animations(
  Scene& scene,
  simdjson::dom::array arr)
{
    std::optional<std::uint32_t> object_id;
    std::optional<ml::vec3> translation;
    std::optional<float> phase_offset;
    std::optional<float> angular_speed;

    for(auto item: arr)
    {
        auto json_object = item.get_object();

        for(auto field: json_object)
        {
            std::string_view key = field.key;

            if(key == "object_id")
            {
                auto id = field.value.get_int64();
                if(id.error())
                {
                    throw std::runtime_error{
                      std::format(
                        "Cannot parse object_id: {}",
                        simdjson::error_message(id.error()))};
                }
                if(id.value() < 0
                   || id.value() > std::numeric_limits<std::uint32_t>::max())
                {
                    throw std::runtime_error{
                      std::format(
                        "Cannot parse object_id: {} out of range [0, {}]",
                        id.value(),
                        std::numeric_limits<std::uint32_t>::max())};
                }

                object_id = static_cast<std::uint32_t>(id.value());
            }
            else if(key == "spin_animation")
            {
                auto json_object = field.value.get_object();
                for(auto& field: json_object)
                {
                    auto key = field.key;

                    if(key == "translation")
                    {
                        auto arr = field.value.get_array();

                        translation = ml::vec3::zero();
                        std::size_t i{0};
                        double d{};

                        for(auto item: arr)
                        {
                            if(item.get_double().get(d) == simdjson::error_code::SUCCESS)
                            {
                                if(i < 3)
                                {
                                    (*translation)[i] = static_cast<float>(d);
                                }
                            }
                            ++i;
                        }

                        if(i != 3)
                        {
                            get_logger().errorf(
                              "Unexpected value count {} when deserializing vec3 'translation' from JSON.",
                              i);
                        }
                    }
                    else if(key == "angular_speed")
                    {
                        auto d = field.value.get_double();
                        if(d.error())
                        {
                            throw std::runtime_error{
                              std::format(
                                "Cannot parse angular_speed: {}",
                                simdjson::error_message(d.error()))};
                        }
                        angular_speed = d.value();
                    }
                    else if(key == "phase_offset")
                    {
                        auto d = field.value.get_double();
                        if(d.error())
                        {
                            throw std::runtime_error{
                              std::format(
                                "Cannot parse phase_offset: {}",
                                simdjson::error_message(d.error()))};
                        }
                        phase_offset = d.value();
                    }
                    else
                    {
                        get_logger().warningf(
                          "Unknown key '{}' found in spin_animation.",
                          key);
                    }
                }
            }
            else
            {
                get_logger().warningf(
                  "Unknown key '{}' found in spin animations.",
                  key);
            }
        }

        if(!angular_speed.has_value()
           || !phase_offset.has_value()
           || !translation.has_value()
           || !object_id.has_value())
        {
            get_logger().errorf(
              "Missing data for spin animation.");
        }
        else
        {
            scene.set_spin_animation(
              {object_id.value()},
              {.translation = translation.value(),
               .angular_speed = angular_speed.value(),
               .phase_offset = phase_offset.value()});
        }

        object_id.reset();
        translation.reset();
        angular_speed.reset();
        phase_offset.reset();
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

        if(key == "objects")
        {
            load_objects(scene, field.value.get_array());
        }
        else if(key == "paused")
        {
            scene.set_paused(field.value.get_bool());
        }
        else if(key == "spin_animations")
        {
            load_spin_animations(scene, field.value.get_array());
        }
        else if(key == "systems")
        {
            load_systems(scene, field.value.get_array());
        }
        else if(key == "time")
        {
            scene.set_time(field.value.get_double());
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

    /*
     * Capture snapshots.
     */

    scene.for_each_object<Object>(
      [](Object& obj)
      { obj.capture_snapshot(); });
}

}    // namespace serial::json

/**
 * Software Rasterizer Playground.
 *
 * Scene description.
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
#include "serialization/json/json_property_writer.h"
#include "systems/animation.h"
#include "systems/lights.h"
#include "systems/object_tick.h"
#include "scene.h"
#include "logging.h"

namespace
{

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"Startup"};
    return logger;
}

class DeserializerVisitor : public reflect::PropertyVisitor
{
    Object& object;
    simdjson::ondemand::value& value;

public:
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
                        if(idx == 0)
                            v.x = static_cast<float>(d);
                        else if(idx == 1)
                            v.y = static_cast<float>(d);
                        else if(idx == 2)
                            v.z = static_cast<float>(d);
                        else if(idx == 3)
                            v.w = static_cast<float>(d);
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
            throw std::runtime_error{
              std::format(
                "Unsupported property type for property '{}'",
                property.get_name())};
        }
    }
};

}    // namespace

Scene::Scene()
{
}

void Scene::clear()
{
    for(auto& obj: objects)
    {
        obj->release();
    }

    objects.clear();
    objects_by_id.clear();
    spin_animations.clear();
}

void Scene::tick(float delta_time)
{
    if(is_paused())
    {
        return;
    }

    // update scene time.
    time += delta_time;

    for(auto& system: systems)
    {
        system->tick(*this, delta_time);
    }
}

void Scene::add_default_systems()
{
    add_system<ObjectTickSystem>();
    add_system<AnimationSystem>();
    add_system<LightSystem>();
}

void Scene::set_spin_animation(
  ObjectId object_id,
  SpinAnimation animation)
{
    spin_animations[object_id] = animation;
}

void Scene::remove_spin_animation(ObjectId object_id)
{
    spin_animations.erase(object_id);
}

Object* Scene::find_object(ObjectId id)
{
    const auto object_it = objects_by_id.find(id);
    if(object_it == objects_by_id.end())
    {
        return nullptr;
    }
    return object_it->second;
}

const Object* Scene::find_object(ObjectId id) const
{
    const auto object_it = objects_by_id.find(id);
    if(object_it == objects_by_id.end())
    {
        return nullptr;
    }
    return object_it->second;
}

Camera* Scene::find_camera(ObjectId id)
{
    return find_object<Camera>(id);
}

const Camera* Scene::find_camera(ObjectId id) const
{
    return find_object<Camera>(id);
}

swr::string Scene::save(
  std::size_t indentation_size,
  bool use_compacted_format) const
{
    serial::json::JsonWriter writer{
      indentation_size,
      use_compacted_format};

    writer.begin_object();

    writer.write_key_value("time", time);
    writer.write_key_value("paused", paused);

    writer.write_key("objects");
    writer.begin_array();

    for(const auto& obj: objects)
    {
        writer.begin_object();

        writer.write_key_value(
          "class",
          obj->get_class()->qualified_name);

        auto& properties = obj->get_properties();
        serial::json::JsonPropertyWriter property_writer{writer};

        for(std::size_t i = 0; i < properties.size(); ++i)
        {
            auto* property = properties[i].get();
            if(property == nullptr)
            {
                continue;
            }

            property->accept(property_writer);
        }

        writer.end_object();
    }

    writer.end_array();
    writer.end_object();

    return writer.get();
}

void Scene::load(
  const std::string_view& text)
{
    clear();

    simdjson::padded_string padded_json{text};
    simdjson::ondemand::parser json_parser;

    simdjson::ondemand::document doc;
    auto error = json_parser.iterate(padded_json).get(doc);
    if(error)
    {
        throw std::runtime_error{
          std::format(
            "JSON parsing failed: {}",
            simdjson::error_message(error))};
    }

    simdjson::ondemand::object root_obj;
    if(doc.get_object().get(root_obj))
    {
        throw std::runtime_error{
          "Root JSON value must be an object."};
    }

    for(auto field: root_obj)
    {
        std::string_view key = field.unescaped_key();

        if(key == "time")
        {
            time = field.value().get_double();
        }
        else if(key == "paused")
        {
            paused = field.value().get_bool();
        }
        else if(key == "objects")
        {
            simdjson::ondemand::array arr = field.value().get_array();

            for(auto item: arr)
            {
                simdjson::ondemand::object obj = item.get_object();

                // Resolve the "class" field first.
                struct FieldEntry
                {
                    std::string_view key;
                    simdjson::ondemand::value value;
                };

                swr::vector<FieldEntry> fields;
                std::string_view qualified_class_name;

                for(auto field: obj)
                {
                    std::string_view key = field.unescaped_key();

                    simdjson::ondemand::value val;
                    if(auto err = field.value().get(val); err)
                    {
                        get_logger().warningf(
                          "Failed to read JSON value for key '{}': {}",
                          key,
                          simdjson::error_message(err));
                        continue;
                    }
                    if(key == "class")
                    {
                        simdjson::error_code error = val.get_string().get(qualified_class_name);
                        if(error)
                        {
                            throw std::runtime_error{
                              "Object entry missing required string field 'class'."};
                        }
                    }
                    else
                    {
                        fields.push_back(FieldEntry{key, val});
                    }
                }

                if(qualified_class_name.empty())
                {
                    throw std::runtime_error{"Object entry missing required field 'class'."};
                }

                // Instantiate the object via reflection.
                auto new_object = reflect::construct<Object>(qualified_class_name);
                if(new_object == nullptr)
                {
                    throw std::runtime_error{
                      std::format(
                        "Object construction failed for class '{}'.",
                        qualified_class_name)};
                }

                swr::unordered_set<std::string_view> processed_keys;
                for(auto& entry: fields)
                {
                    // TODO Use std::unordered_map for look-up.
                    auto prop_it = std::ranges::find_if(
                      new_object->get_properties(), [key = entry.key](const auto& p)
                      { return p->get_name() == key; });

                    if(prop_it != new_object->get_properties().end())
                    {
                        auto& prop = *prop_it;

                        DeserializerVisitor visitor{*new_object, entry.value};
                        prop->accept(visitor);

                        processed_keys.insert(entry.key);
                    }
                    else
                    {
                        get_logger().warningf(
                          "Unknown field '{}' found for class '{}'",
                          entry.key, new_object->get_class()->qualified_name);
                    }
                }

                // Add constructed object to the scene
                objects.emplace_back(std::move(new_object));
            }
        }
        else
        {
            get_logger().warningf(
              "Unknown key '{}' found in scene",
              key);
        }
    }
}

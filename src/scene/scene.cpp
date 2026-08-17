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
#include "serialization/json/property_writer.h"
#include "serialization/json/writer.h"
#include "systems/animation.h"
#include "systems/lights.h"
#include "systems/object_tick.h"
#include "scene.h"
#include "logging.h"

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

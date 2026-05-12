/**
 * Software Rasterizer Playground.
 *
 * Scene description.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "reflection/cast.h"
#include "systems/animation.h"
#include "systems/object_tick.h"
#include "gear.h"
#include "scene.h"

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
    for(auto& object: objects)
    {
        if(object->get_object_id() != id)
        {
            continue;
        }

        return reflect::try_cast<Camera, Object>(object.get());
    }
    return nullptr;
}

const Camera* Scene::find_camera(ObjectId id) const
{
    for(const auto& object: objects)
    {
        if(object->get_object_id() != id)
        {
            continue;
        }

        return reflect::try_cast<Camera, Object>(object.get());
    }
    return nullptr;
}

std::vector<Camera*> Scene::get_cameras()
{
    std::vector<Camera*> cameras;
    for(auto& object: objects)
    {
        if(auto* camera = reflect::try_cast<Camera, Object>(object.get()))
        {
            cameras.push_back(camera);
        }
    }
    return cameras;
}

std::vector<const Camera*> Scene::get_cameras() const
{
    std::vector<const Camera*> cameras;
    for(const auto& object: objects)
    {
        if(const auto* camera = reflect::try_cast<Camera, Object>(object.get()))
        {
            cameras.push_back(camera);
        }
    }
    return cameras;
}

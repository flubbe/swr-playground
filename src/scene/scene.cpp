/**
 * Software Rasterizer Playground.
 *
 * Scene description.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "systems/animation.h"
#include "systems/lights.h"
#include "systems/object_tick.h"
#include "scene.h"

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

void Scene::get_cameras(
  swr::vector<Camera*>& cameras)
{
    cameras.clear();
    for_each_object<Camera>(
      [&cameras](Camera& camera)
      {
          cameras.push_back(&camera);
      });
}

void Scene::get_cameras(
  swr::vector<const Camera*>& cameras) const
{
    cameras.clear();
    for_each_object<Camera>(
      [&cameras](const Camera& camera)
      {
          cameras.push_back(&camera);
      });
}

void Scene::get_directional_lights(
  swr::vector<DirectionalLight*>& lights)
{
    lights.clear();
    for_each_object<DirectionalLight>(
      [&lights](DirectionalLight& light)
      {
          lights.push_back(&light);
      });
}

void Scene::get_directional_lights(
  swr::vector<const DirectionalLight*>& lights) const
{
    lights.clear();
    for_each_object<DirectionalLight>(
      [&lights](const DirectionalLight& light)
      {
          lights.push_back(&light);
      });
}

void Scene::get_spot_lights(
  swr::vector<SpotLight*>& lights)
{
    lights.clear();
    for_each_object<SpotLight>(
      [&lights](SpotLight& light)
      {
          lights.push_back(&light);
      });
}

void Scene::get_spot_lights(
  swr::vector<const SpotLight*>& lights) const
{
    lights.clear();
    for_each_object<SpotLight>(
      [&lights](const SpotLight& light)
      {
          lights.push_back(&light);
      });
}

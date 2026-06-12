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

namespace
{

void configure_default_directional_lights(Scene& scene)
{
    auto* key_light = scene.add_object<DirectionalLight>();
    key_light->set_name("Directional Light 1");
    key_light->behavior = DirectionalLightBehavior::Rotating;
    key_light->brightness = 0.55f;
    key_light->set_transform(
      ml::matrices::rotation_y(ml::to_radians(210.f))
      * ml::matrices::rotation_x(ml::to_radians(-35.f)));
    key_light->set_position({5.f, 8.f, 10.f});
    key_light->capture_snapshot();

    auto* fill_light = scene.add_object<DirectionalLight>();
    fill_light->set_name("Directional Light 2");
    fill_light->behavior = DirectionalLightBehavior::Stationary;
    fill_light->brightness = 0.25f;
    fill_light->set_transform(
      ml::matrices::rotation_y(ml::to_radians(35.f))
      * ml::matrices::rotation_x(ml::to_radians(-55.f)));
    fill_light->set_position({-10.f, 12.f, -6.f});
    fill_light->capture_snapshot();
}

}    // namespace

Scene::Scene()
{
    configure_default_directional_lights(*this);
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

    configure_default_directional_lights(*this);
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

std::vector<Camera*> Scene::get_cameras()
{
    std::vector<Camera*> cameras;
    for_each_object<Camera>(
      [&cameras](Camera& camera)
      {
          cameras.push_back(&camera);
      });
    return cameras;
}

std::vector<const Camera*> Scene::get_cameras() const
{
    std::vector<const Camera*> cameras;
    for_each_object<Camera>(
      [&cameras](const Camera& camera)
      {
          cameras.push_back(&camera);
      });
    return cameras;
}

std::vector<DirectionalLight*> Scene::get_directional_lights()
{
    std::vector<DirectionalLight*> lights;
    for_each_object<DirectionalLight>(
      [&lights](DirectionalLight& light)
      {
          lights.push_back(&light);
      });
    return lights;
}

std::vector<const DirectionalLight*> Scene::get_directional_lights() const
{
    std::vector<const DirectionalLight*> lights;
    for_each_object<DirectionalLight>(
      [&lights](const DirectionalLight& light)
      {
          lights.push_back(&light);
      });
    return lights;
}

std::vector<SpotLight*> Scene::get_spot_lights()
{
    std::vector<SpotLight*> lights;
    for_each_object<SpotLight>(
      [&lights](SpotLight& light)
      {
          lights.push_back(&light);
      });
    return lights;
}

std::vector<const SpotLight*> Scene::get_spot_lights() const
{
    std::vector<const SpotLight*> lights;
    for_each_object<SpotLight>(
      [&lights](const SpotLight& light)
      {
          lights.push_back(&light);
      });
    return lights;
}

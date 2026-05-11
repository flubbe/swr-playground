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
#include "gear.h"
#include "scene.h"

namespace
{

class ObjectTickSystem final : public SceneSystem
{
public:
    void tick(
      Scene& scene,
      float delta_time) override
    {
        for(auto& object: scene.get_objects())
        {
            object->tick(delta_time);
        }
    }
};

}    // namespace

void Scene::clear()
{
    for(auto& obj: objects)
    {
        obj->release();
    }

    objects.clear();
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

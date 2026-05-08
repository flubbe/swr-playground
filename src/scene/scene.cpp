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
    if(!is_paused())
    {
        // update scene time.
        time += delta_time;
    }
}

Camera* Scene::find_camera(ObjectId id)
{
    if(id.value == 0)
    {
        return nullptr;
    }

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
    if(id.value == 0)
    {
        return nullptr;
    }

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

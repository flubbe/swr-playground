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
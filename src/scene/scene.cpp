/**
 * Software Rasterizer Playground.
 *
 * Scene description.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

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

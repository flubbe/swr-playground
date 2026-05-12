/**
 * Software Rasterizer Playground.
 *
 * Object update tick system.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "scene/scene.h"
#include "system.h"

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

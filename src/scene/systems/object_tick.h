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

struct ObjectTickSystem final
: public SceneSystem
{
    std::string_view get_name() const override
    {
        return "Tick";
    }

    void tick(
      Scene& scene,
      float delta_time) override
    {
        scene.for_each_object<Object>(
          [delta_time](Object& object)
          {
              object.tick(delta_time);
          });
    }
};

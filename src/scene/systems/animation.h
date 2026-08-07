/**
 * Software Rasterizer Playground.
 *
 * Animation system.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <ml/all.h>

#include "scene/scene.h"
#include "system.h"

class AnimationSystem final : public SceneSystem
{
public:
    void tick(
      Scene& scene,
      [[maybe_unused]] float delta_time) override
    {
        const float time = scene.get_time();
        const auto& spin_animations = scene.get_spin_animations();
        const auto& object_index = scene.get_objects_by_id();

        for(const auto& [object_id, animation]: spin_animations)
        {
            const auto object_it = object_index.find(object_id);
            if(object_it == object_index.end())
            {
                continue;
            }

            const float angle =
              animation.angular_speed * time + animation.phase_offset;
            object_it->second->set_transform(
              ml::matrices::translation(animation.translation)
              * ml::matrices::rotation_z(angle));
        }
    }
};

/**
 * Software Rasterizer Playground.
 *
 * Light system.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cmath>

#include "scene/scene.h"
#include "system.h"

class LightSystem final : public SceneSystem
{
    constexpr static float orbit_radius = 10.f;
    constexpr static float orbit_height = 8.f;
    constexpr static float orbit_speed = 0.5f;

public:
    void tick(
      Scene& scene,
      [[maybe_unused]] float delta_time) override
    {
        Light& light = scene.get_light();
        if(light.type == LightType::Stationary)
        {
            light.position = {120.f, 90.f, 120.f, 1.f};
            return;
        }

        const float angle = scene.get_time() * orbit_speed;
        light.position = {
          orbit_radius * std::cos(angle),
          orbit_height,
          orbit_radius * std::sin(angle),
          1.f};
    }
};

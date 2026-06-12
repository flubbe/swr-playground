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

#include "ml/all.h"

#include "scene/directionallight.h"
#include "scene/scene.h"
#include "system.h"

class LightSystem final : public SceneSystem
{
    constexpr static float orbit_radius = 10.f;
    constexpr static float orbit_height = 8.f;
    constexpr static float orbit_speed = 0.5f;
    constexpr static float orbit_pitch_radians = -0.7f;

    static ml::mat4x4 make_orbit_orientation(
      float angle,
      float pitch_radians)
    {
        return ml::matrices::rotation_y(angle + static_cast<float>(M_PI))
               * ml::matrices::rotation_x(pitch_radians);
    }

    static ml::mat4x4 make_stationary_orientation()
    {
        return ml::matrices::rotation_y(ml::to_radians(35.f))
               * ml::matrices::rotation_x(ml::to_radians(-55.f));
    }

public:
    void tick(
      Scene& scene,
      [[maybe_unused]] float delta_time) override
    {
        auto directional_lights = scene.get_directional_lights();
        for(std::size_t light_index = 0; light_index < directional_lights.size(); ++light_index)
        {
            DirectionalLight& light = *directional_lights[light_index];
            if(light.behavior == DirectionalLightBehavior::Stationary)
            {
                light.set_transform(make_stationary_orientation());
                light.set_position({-10.f, 12.f, -6.f});
                continue;
            }

            const float phase_offset =
              (light_index == 0)
                ? 0.f
                : static_cast<float>(M_PI);
            const float angle = scene.get_time() * orbit_speed + phase_offset;
            light.set_transform(make_orbit_orientation(angle, orbit_pitch_radians));
            light.set_position({
              (orbit_radius + static_cast<float>(light_index) * 2.f) * std::cos(angle),
              orbit_height + static_cast<float>(light_index) * 2.f,
              (orbit_radius + static_cast<float>(light_index) * 2.f) * std::sin(angle)});
        }
    }
};

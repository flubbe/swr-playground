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

#include <algorithm>
#include <cmath>

#include "ml/all.h"

#include "scene/directionallight.h"
#include "scene/scene.h"
#include "scene/spotlight.h"
#include "system.h"

class LightSystem final : public SceneSystem
{
    constexpr static float orbit_radius = 10.f;
    constexpr static float orbit_height = 8.f;
    constexpr static float orbit_speed = 0.5f;
    constexpr static float orbit_pitch_radians = -0.7f;
    constexpr static float spot_orbit_radius = 16.f;
    constexpr static float spot_orbit_height = 11.f;
    constexpr static float spot_orbit_speed = 0.5f;

    static ml::mat4x4 make_spot_orbit_orientation(
      const ml::vec3& position)
    {
        const ml::vec3 direction_to_origin =
          (-position).normalized();
        const float pitch = std::asin(direction_to_origin.y);
        const float yaw =
          std::atan2(-direction_to_origin.x, -direction_to_origin.z);
        return ml::matrices::rotation_y(yaw)
               * ml::matrices::rotation_x(pitch);
    }

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

        auto spot_lights = scene.get_spot_lights();
        for(std::size_t light_index = 0; light_index < spot_lights.size(); ++light_index)
        {
            SpotLight& light = *spot_lights[light_index];
            const float phase_offset =
              static_cast<float>(light_index)
              * (2.f * static_cast<float>(M_PI)
                 / std::max<std::size_t>(spot_lights.size(), 1));
            const float angle =
              scene.get_time() * spot_orbit_speed + phase_offset;
            const ml::vec3 position = {
              spot_orbit_radius * std::cos(angle),
              spot_orbit_height,
              spot_orbit_radius * std::sin(angle)};
            light.set_transform(make_spot_orbit_orientation(position));
            light.set_position(position);
        }
    }
};

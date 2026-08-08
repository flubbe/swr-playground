/**
 * Software Rasterizer Playground.
 *
 * Phong lighting shader.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <swr/swr.h>
#include <swr/shaders.h>

#include "shader_constants.h"

namespace shader
{

/**
 * A Phong shader with up to `max_directional_lights` directional lights
 * and up to `max_spot_lights` spot lights.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *   attribute 1: vertex normal
 *
 * varyings:
 *   location 0: normal in camera space [vec4] [smooth]
 *
 * uniforms:
 *   location camera_projection_uniform_index: projection matrix [mat4x4]
 *   location camera_view_uniform_index: view matrix [mat4x4]
 *   location directional_light_count_uniform_index: directional light count [int]
 *   location directional_light_uniform_base_index..
 *            (directional_light_uniform_base_index + max_directional_lights - 1):
 *            directional lights in camera space, brightness in w [vec4]
 *   location spot_light_count_uniform_index: spot light count [int]
 *   location spot_light_uniform_base_index..: spot light tuples:
 *      position/range, direction/brightness, params(inner/outer cone) [vec4]
 *
 */

class PhongSmooth final
: public swr::program<PhongSmooth>
{
    static constexpr float ambient_strength = 0.15f;
    static constexpr float specular_strength = 0.5f;
    static constexpr float shininess = 64.f;

public:
    static constexpr std::string_view name = "PhongSmooth";

    PhongSmooth() = default;

    swr::program_metadata get_metadata() const override
    {
        return {
          .fragment_shader_may_discard = false,
          .fragment_shader_may_write_depth = false};
    }

    void pre_link(
      boost::container::static_vector<
        swr::interpolation_qualifier,
        swr::limits::max::varyings>& iqs) const override
    {
        iqs = {
          swr::interpolation_qualifier::smooth, /* position */
          swr::interpolation_qualifier::smooth, /* normal */
        };
    }

    void vertex_shader(
      [[maybe_unused]] int gl_VertexID,
      [[maybe_unused]] int gl_InstanceID,
      std::span<const ml::vec4> attribs,
      ml::vec4& gl_Position,
      [[maybe_unused]] float& gl_PointSize,
      [[maybe_unused]] std::span<float> gl_ClipDistance,
      std::span<ml::vec4> varyings) const override
    {
        ml::mat4x4 proj = uniforms[camera_projection_uniform_index].m4;
        ml::mat4x4 view = uniforms[camera_view_uniform_index].m4;
        const ml::vec4 position_cameraspace = view * attribs[0];
        gl_Position = proj * position_cameraspace;

        varyings[0] = ml::vec4(position_cameraspace.xyz(), 0.f); /* position in camera space */
        varyings[1] = ml::vec4((view * attribs[1]).xyz(), 0.f);  /* normal in camera space */
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      ml::vec4& gl_FragColor) const override
    {
        const ml::vec3 position_cameraspace = ml::vec4(varyings[0]).xyz();
        const ml::vec3 N = ml::vec4(varyings[1]).xyz().normalized();
        const int light_count = uniforms[directional_light_count_uniform_index].i;
        const auto lights = load_directional_lights(uniforms);
        const int spot_light_count = uniforms[spot_light_count_uniform_index].i;
        const ml::vec4 diffuse_color = uniforms[material_color_uniform_index].v4;
        const ml::vec4 ambient_color = diffuse_color;

        float diff = 0.f;
        const int active_light_count = clamp_light_count(light_count);
        for(int light_index = 0; light_index < active_light_count; ++light_index)
        {
            const ml::vec4& directional_light =
              lights[static_cast<std::size_t>(light_index)];
            diff +=
              std::max(ml::dot(N, directional_light.xyz()), 0.f)
              * directional_light.w;
        }
        diff = std::clamp(diff, 0.f, 1.f);

        const ml::vec3 view_dir = (-position_cameraspace).normalized();
        float spec = 0.f;
        for(int light_index = 0; light_index < active_light_count; ++light_index)
        {
            const ml::vec4& directional_light =
              lights[static_cast<std::size_t>(light_index)];
            const ml::vec3 light_dir = directional_light.xyz();
            const ml::vec3 reflect_dir =
              (-light_dir + N * (2.f * ml::dot(N, light_dir))).normalized();
            spec +=
              std::pow(std::max(ml::dot(reflect_dir, view_dir), 0.f), shininess)
              * directional_light.w;
        }
        spec = std::clamp(spec, 0.f, 1.f);

        const LightContribution spot = phong::accumulate_spot_lights(
          N,
          position_cameraspace,
          view_dir,
          spot_light_count,
          uniforms,
          shininess);
        const ml::vec4 ambient = ambient_color * ambient_strength;
        const ml::vec4 diffuse = diffuse_color * diff + diffuse_color * spot.diffuse;
        const ml::vec4 specular =
          ml::vec4{1.f, 1.f, 1.f, 0.f} * (specular_strength * spec)
          + spot.specular * specular_strength;

        gl_FragColor = ml::clamp_to_unit_interval(ambient + diffuse + specular + ml::vec4{0.f, 0.f, 0.f, 1.f});

        return swr::accept;
    }
};

}    // namespace shader
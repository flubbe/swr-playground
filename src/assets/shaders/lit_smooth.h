/**
 * Software Rasterizer Playground.
 *
 * Smoothly lit and shadowed shader.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <numbers>

#include <swr/swr.h>
#include <swr/shaders.h>

#include "common.h"
#include "shader_constants.h"

namespace shader
{
/**
 * Smoothly lit and shadowed shader.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *   attribute 1: vertex normal
 *
 * varyings:
 *   location 0: position in camera space [vec4] [smooth]
 *   location 1: normal in camera space   [vec4] [smooth]
 *   location 2: shadow clip position     [vec4] [smooth]
 *
 * uniforms:
 *   location `camera_projection_uniform_index`: projection matrix [mat4x4]
 *   location `camera_view_uniform_index`: view matrix [mat4x4]
 *   location `directional_light_count_uniform_index`: directional light count [int]
 *   location `directional_light_uniform_base_index..`: directional lights in camera space, brightness in `w` [vec4]
 *   location `spot_light_count_uniform_index`: spot light count [int]
 *   location `spot_light_uniform_base_index..`: spot light tuples: position/range, direction/brightness, params(inner/outer cone) [vec4]
 *   location `shadow_map_matrix_uniform_index`: shadow map projection matrix [mat4x4]
 *   location `shadow_map_params_uniform_index`: shadow sampling params (bias, pcf_mode, unused, unused) [vec4]
 *   location `material_color_uniform_index`: material color [vec4]
 */
class LitSmooth final
: public swr::program<LitSmooth>
{
    static constexpr float ambient_strength = 0.15f;

    // Dielectric-ish default reflectance.
    static constexpr float specular_strength = 0.04f;

    static constexpr float shininess = 64.f;
    static constexpr float normalized_specular_scale =
      (shininess + 8.f) / (8.f * std::numbers::pi_v<float>);

public:
    static constexpr std::string_view name = "LitSmooth";

    LitSmooth() = default;

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
          swr::interpolation_qualifier::smooth, /* shadow clip position */
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

        varyings[0] = ml::vec4(position_cameraspace.xyz(), 0.f);
        varyings[1] = ml::vec4((view * attribs[1]).xyz(), 0.f);
        varyings[2] = uniforms[shadow_map_matrix_uniform_index].m4 * attribs[0];
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

        const int light_count =
          clamp_light_count(uniforms[directional_light_count_uniform_index].i);
        const int spot_light_count =
          clamp_spot_light_count(uniforms[spot_light_count_uniform_index].i);
        const ml::vec4 shadow_params =
          uniforms[shadow_map_params_uniform_index].v4;
        const int shadow_pcf_mode =
          static_cast<int>(shadow_params.y + 0.5f);
        const float shadow =
          spot_light_count > 0
            ? sample_shadow_map(
                uniforms,
                sampler2D(shadow_map_sampler_unit),
                sampler2DShadow(shadow_map_sampler_unit),
                varyings[2],
                shadow_params.x,
                shadow_pcf_mode)
            : 1.f;

        const ml::vec4 material_color = uniforms[material_color_uniform_index].v4;

        const ml::vec4 diffuse_color =
          material_color * (1.f - specular_strength);

        const ml::vec4 specular_color{
          specular_strength,
          specular_strength,
          specular_strength,
          0.f};

        const ml::vec3 view_dir = (-position_cameraspace).normalized();

        float directional_diffuse = 0.f;
        float directional_specular = 0.f;

        for(int light_index = 0; light_index < light_count; ++light_index)
        {
            const ml::vec4 directional_light =
              get_directional_light(uniforms, static_cast<std::size_t>(light_index));

            const ml::vec3 light_dir = directional_light.xyz();
            const float brightness = directional_light.w;

            const float NoL = std::max(ml::dot(N, light_dir), 0.f);
            directional_diffuse += NoL * brightness;

            if(NoL > 0.f)
            {
                const ml::vec3 half_dir = (light_dir + view_dir).normalized();
                const float NoH = std::max(ml::dot(N, half_dir), 0.f);

                directional_specular +=
                  normalized_specular_scale
                  * std::pow(NoH, shininess)
                  * NoL
                  * brightness;
            }
        }

        directional_diffuse = std::clamp(directional_diffuse, 0.f, 1.f);
        directional_specular = std::clamp(directional_specular, 0.f, 1.f);

        const LightContribution spot = lit::accumulate_spot_lights(
          N,
          position_cameraspace,
          view_dir,
          spot_light_count,
          uniforms,
          shininess);
        const ml::vec4 shadowed_spot_diffuse = spot.diffuse * shadow;
        const ml::vec4 shadowed_spot_specular = spot.specular * shadow;

        const ml::vec4 ambient =
          material_color * ambient_strength;

        const ml::vec4 diffuse =
          diffuse_color * directional_diffuse
          + diffuse_color * shadowed_spot_diffuse;

        const ml::vec4 specular =
          specular_color * directional_specular
          + shadowed_spot_specular * specular_strength;

        gl_FragColor = ml::clamp_to_unit_interval(
          ambient
          + diffuse
          + specular
          + ml::vec4{0.f, 0.f, 0.f, material_color.w});

        return swr::accept;
    }
};

}    // namespace shader

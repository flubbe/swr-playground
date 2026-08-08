/**
 * Software Rasterizer Playground.
 *
 * Floor shader with more specular highlights.
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
 * Floor shader with more specular highlights and up to `max_directional_lights`
 * directional lights and up to `max_spot_lights` spot lights.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *   attribute 2: texture coordinates (uv)
 *
 * varyings:
 *   location 0: uv (texture coordinates)                  [smooth]
 *   location 1: position in camera space                  [smooth]
 *   location 2: normal in camera space                    [smooth]
 *   location 3: tangent in camera space                   [smooth]
 *   location 4: bitangent in camera space                 [smooth]
 *   location 5: eye direction in camera space             [smooth]
 *   location 6: shadow clip position                      [smooth]
 *
 * uniforms / samplers:
 *   location `camera_projection_uniform_index`: projection matrix [mat4x4]
 *   location `camera_view_uniform_index`: view matrix [mat4x4]
 *   location `shadow_map_matrix_uniform_index`: shadow map projection matrix [mat4x4]
 *   location `shadow_map_params_uniform_index`: shadow sampling params [vec4]
 *   sampler unit 0: base color texture (sampler2D)
 *   sampler unit 1: normal map texture (sampler2D)
 *   location `directional_light_count_uniform_index`: directional light count [int]
 *   location `spot_light_count_uniform_index`: spot light count [int]
 */
class TexturedShinyFloor final
: public swr::program<TexturedShinyFloor>
{
    const ml::vec4 light_color{1.f, 1.f, 1.f, 1.f};
    const ml::vec4 light_specular_color{1.f, 1.f, 1.f, 1.f};
    static constexpr float ambient_diffuse_factor = 0.10f;
    static constexpr float specular_strength = 0.35f;
    static constexpr float shininess = 24.f;

public:
    static constexpr std::string_view name = "TexturedShinyFloor";

    TexturedShinyFloor() = default;

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
          swr::interpolation_qualifier::smooth, /* uv */
          swr::interpolation_qualifier::smooth, /* position in camera space */
          swr::interpolation_qualifier::smooth, /* normal in camera space */
          swr::interpolation_qualifier::smooth, /* tangent in camera space */
          swr::interpolation_qualifier::smooth, /* bitangent in camera space */
          swr::interpolation_qualifier::smooth, /* eye direction in camera space */
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
        const ml::mat4x4 proj = uniforms[camera_projection_uniform_index].m4;
        const ml::mat4x4 view = uniforms[camera_view_uniform_index].m4;
        const ml::vec3 position_cameraspace = (view * attribs[0]).xyz();
        const ml::vec3 normal_cameraspace =
          (view * ml::vec4{0.f, 1.f, 0.f, 0.f}).xyz();
        const ml::vec3 tangent_cameraspace =
          (view * ml::vec4{1.f, 0.f, 0.f, 0.f}).xyz();
        const ml::vec3 bitangent_cameraspace =
          (view * ml::vec4{0.f, 0.f, -1.f, 0.f}).xyz();

        gl_Position = proj * view * attribs[0];
        varyings[0] = attribs[2];
        varyings[1] = ml::vec4(position_cameraspace, 0.f);
        varyings[2] = ml::vec4(normal_cameraspace, 0.f);
        varyings[3] = ml::vec4(tangent_cameraspace, 0.f);
        varyings[4] = ml::vec4(bitangent_cameraspace, 0.f);
        varyings[5] = ml::vec4(-position_cameraspace, 0.f);
        varyings[6] = uniforms[shadow_map_matrix_uniform_index].m4 * attribs[0];
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      ml::vec4& gl_FragColor) const override
    {
        const swr::varying& tex_coords = varyings[0];
        const ml::vec3 position_cameraspace = ml::vec4(varyings[1]).xyz();
        const ml::vec3 normal = ml::vec4(varyings[2]).xyz();
        const ml::vec3 tangent = ml::vec4(varyings[3]).xyz();
        const ml::vec3 bitangent = ml::vec4(varyings[4]).xyz();
        const ml::vec3 eye_direction = ml::vec4(varyings[5]).xyz();
        const int spot_light_count = clamp_spot_light_count(
          uniforms[spot_light_count_uniform_index].i);
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
                varyings[6],
                shadow_params.x,
                shadow_pcf_mode)
            : 1.f;

        const ml::vec4 base_color = sampler2D(0).sample_at(tex_coords);
        const ml::vec3 material_normal =
          (sampler2D(1).sample_at(tex_coords) * 2.f - 1.f).xyz();

        const ml::vec3 N =
          (tangent * material_normal.x
           + bitangent * material_normal.y
           + normal * material_normal.z)
            .normalized();
        const ml::vec3 view_dir = eye_direction.normalized();

        const int light_count = clamp_light_count(
          uniforms[directional_light_count_uniform_index].i);

        float lambertian_sum = 0.f;
        float specular_sum = 0.f;
        for(int light_index = 0; light_index < light_count; ++light_index)
        {
            const ml::vec4 directional_light =
              get_directional_light(uniforms, static_cast<std::size_t>(light_index));
            const ml::vec3 light_dir = directional_light.xyz();
            const float lambertian =
              std::clamp(ml::dot(N, light_dir), 0.f, 1.f);
            lambertian_sum += lambertian * directional_light.w;

            if(lambertian > 0.f)
            {
                const ml::vec3 reflect_dir =
                  -(light_dir - N * 2.f * ml::dot(light_dir, N));
                const float specular_angle =
                  ml::dot(reflect_dir, view_dir);
                specular_sum +=
                  std::pow(std::clamp(specular_angle, 0.f, 1.f), shininess)
                  * directional_light.w;
            }
        }

        lambertian_sum = std::clamp(lambertian_sum, 0.f, 1.f);
        specular_sum = std::clamp(specular_sum, 0.f, 1.f);
        const LightContribution spot = phong::accumulate_spot_lights(
          N,
          position_cameraspace,
          view_dir,
          spot_light_count,
          uniforms,
          shininess);
        const ml::vec4 shadowed_spot_diffuse = spot.diffuse * shadow;
        const ml::vec4 shadowed_spot_specular = spot.specular * shadow;

        const ml::vec4 ambient_color = base_color * ambient_diffuse_factor;
        const ml::vec4 diffuse_color =
          light_color * base_color * lambertian_sum
          + base_color * shadowed_spot_diffuse;
        const ml::vec4 specular_color =
          light_specular_color * (specular_strength * specular_sum)
          + shadowed_spot_specular * specular_strength;

        gl_FragColor = ml::clamp_to_unit_interval(ambient_color + diffuse_color + specular_color);
        gl_FragColor.w = base_color.w;
        return swr::accept;
    }
};

}    // namespace shader

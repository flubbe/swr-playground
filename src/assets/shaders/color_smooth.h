/**
 * Software Rasterizer Playground.
 *
 * Smooth shaded color shader.
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
 * Smooth shaded color shader.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *   attribute 1: vertex normal
 *
 * varyings:
 *   location 0: position in camera space [vec4] [smooth]
 *   location 1: normal in camera space   [vec4] [smooth]
 *   location 2..(2 + max_directional_lights - 1): directional light tuples [vec4] [flat]
 *
 * uniforms:
 *   location `camera_projection_uniform_index`: projection matrix [mat4x4]
 *   location `camera_view_uniform_index`: view matrix [mat4x4]
 *   location `directional_light_count_uniform_index`: directional light count [int]
 *   location `directional_light_uniform_base_index..(directional_light_uniform_base_index + max_directional_lights - 1)`: directional lights in camera space, brightness in `w` [vec4]
 *   location `spot_light_count_uniform_index`: spot light count [int]
 *   location `spot_light_uniform_base_index..`: spot light tuples: position/range, direction/brightness, params(inner/outer cone) [vec4]
 *   location `material_color_uniform_index`: material color [vec4]
 */
struct ColorSmooth final
: public swr::program<ColorSmooth>
{
    static constexpr std::string_view name = "ColorSmooth";

    ColorSmooth() = default;

    swr::program_metadata get_metadata() const override
    {
        return {
          .fragment_shader_may_discard = false,
          .fragment_shader_may_write_depth = false};
    }

    virtual void pre_link(
      boost::container::static_vector<
        swr::interpolation_qualifier,
        swr::limits::max::varyings>& iqs) const override
    {
        // set interpolation qualifiers for all varyings.
        iqs = {
          swr::interpolation_qualifier::smooth, /* position */
          swr::interpolation_qualifier::smooth, /* normal */
          swr::interpolation_qualifier::flat,   /* light direction 0 */
          swr::interpolation_qualifier::flat,   /* light direction 1 */
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
        const int light_count =
          clamp_light_count(uniforms[directional_light_count_uniform_index].i);
        const auto lights = load_directional_lights(uniforms);
        const ml::vec4 pos_cam = view * attribs[0];

        // transform vertex.
        gl_Position = proj * pos_cam;

        varyings[0] = ml::vec4(pos_cam.xyz(), 0.f);             /* position in camera space */
        varyings[1] = ml::vec4((view * attribs[1]).xyz(), 0.f); /* normal in camera space */
        for(int light_index = 0; light_index < light_count; ++light_index)
        {
            varyings[2 + light_index] = lights[static_cast<std::size_t>(light_index)];
        }
        for(std::size_t light_index = static_cast<std::size_t>(light_count);
            light_index < max_directional_lights;
            ++light_index)
        {
            varyings[2 + light_index] = ml::vec4{0.f, 0.f, 0.f, 0.f};
        }
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      ml::vec4& gl_FragColor) const override
    {
        const ml::vec4 position = varyings[0];
        const ml::vec4 normal = varyings[1];
        const ml::vec4 direction0 = varyings[2];
        const ml::vec4 direction1 = varyings[3];
        const int light_count = uniforms[directional_light_count_uniform_index].i;
        const int spot_light_count = uniforms[spot_light_count_uniform_index].i;
        const ml::vec4 diffuse_color = uniforms[material_color_uniform_index].v4;
        const ml::vec4 ambient_color = diffuse_color;
        const std::array<ml::vec4, max_directional_lights> lights = {
          direction0,
          direction1};

        const ml::vec3 n = normal.xyz().normalized();
        const float l = accumulate_directional_light(
          n,
          light_count,
          lights);
        const LightContribution spot = phong::accumulate_spot_lights(
          n,
          position.xyz(),
          (-position.xyz()).normalized(),
          spot_light_count,
          uniforms,
          1.f);

        // write color.
        gl_FragColor =
          ml::clamp_to_unit_interval(ambient_color * 0.2f + diffuse_color * l + diffuse_color * spot.diffuse);

        // accept fragment.
        return swr::accept;
    }
};

}    // namespace shader
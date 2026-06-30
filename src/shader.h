/**
 * Software Rasterizer Playground.
 *
 * shaders.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>

#include "shader_constants.h"
#include "swr/swr.h"
#include "swr/shaders.h"

namespace shader
{

inline int clamp_light_count(
  int light_count)
{
    return std::clamp(
      light_count,
      0,
      static_cast<int>(max_lights));
}

inline std::array<ml::vec4, max_lights> load_directional_lights(
  std::span<const swr::uniform> uniforms)
{
    std::array<ml::vec4, max_lights> lights{};
    for(std::size_t light_index = 0; light_index < lights.size(); ++light_index)
    {
        lights[light_index] =
          uniforms[directional_light_uniform_index(light_index)].v4;
    }
    return lights;
}

inline int clamp_spot_light_count(
  int light_count)
{
    return std::clamp(
      light_count,
      0,
      static_cast<int>(max_spot_lights));
}

struct SpotLightData
{
    ml::vec4 position_and_range;
    ml::vec4 direction_and_brightness;
    ml::vec4 params;
    ml::vec4 color;
};

inline std::array<SpotLightData, max_spot_lights> load_spot_lights(
  std::span<const swr::uniform> uniforms)
{
    std::array<SpotLightData, max_spot_lights> lights{};
    for(std::size_t light_index = 0; light_index < lights.size(); ++light_index)
    {
        lights[light_index] = {
          .position_and_range =
            uniforms[spot_light_uniform_index(
                       light_index,
                       spot_light_position_uniform_offset)]
              .v4,
          .direction_and_brightness =
            uniforms[spot_light_uniform_index(
                       light_index,
                       spot_light_direction_uniform_offset)]
              .v4,
          .params =
            uniforms[spot_light_uniform_index(
                       light_index,
                       spot_light_params_uniform_offset)]
              .v4,
          .color =
            uniforms[spot_light_uniform_index(
                       light_index,
                       spot_light_color_uniform_offset)]
              .v4,
        };
    }
    return lights;
}

// Helper: Bilinear comparison of depth (emulates sampler2DShadow)
// Samples 4 texels around a fractional uv coordinate and bilinearly blends comparison results
inline float shadow_compare_bilinear(
  const swr::sampler_2d* shadow_map,
  const ml::vec2& uv,
  float receiver_depth,
  float depth_bias)
{
    const ml::tvec2<int> shadow_map_size = shadow_map->size(0);
    const float texel_u = 1.f / shadow_map_size.x;
    const float texel_v = 1.f / shadow_map_size.y;

    // Find the fractional part for interpolation
    ml::vec2 uv_texel{uv.x / texel_u, uv.y / texel_v};
    ml::vec2 frac{
      uv_texel.x - std::floor(uv_texel.x),
      uv_texel.y - std::floor(uv_texel.y),
    };
    ml::vec2 one_minus_frac{1.f - frac.x, 1.f - frac.y};

    // Sample 4 texels
    float c00, c10, c01, c11;
    {
        const swr::varying uv_varying{
          ml::vec4{uv.x - frac.x * texel_u, uv.y - frac.y * texel_v, 0.f, 0.f},
          ml::vec4::zero(),
          ml::vec4::zero(),
        };
        c00 = receiver_depth - depth_bias <= shadow_map->sample_at(uv_varying).x ? 1.f : 0.f;
    }
    {
        const swr::varying uv_varying{
          ml::vec4{uv.x + (1.f - frac.x) * texel_u, uv.y - frac.y * texel_v, 0.f, 0.f},
          ml::vec4::zero(),
          ml::vec4::zero(),
        };
        c10 = receiver_depth - depth_bias <= shadow_map->sample_at(uv_varying).x ? 1.f : 0.f;
    }
    {
        const swr::varying uv_varying{
          ml::vec4{uv.x - frac.x * texel_u, uv.y + (1.f - frac.y) * texel_v, 0.f, 0.f},
          ml::vec4::zero(),
          ml::vec4::zero(),
        };
        c01 = receiver_depth - depth_bias <= shadow_map->sample_at(uv_varying).x ? 1.f : 0.f;
    }
    {
        const swr::varying uv_varying{
          ml::vec4{uv.x + (1.f - frac.x) * texel_u, uv.y + (1.f - frac.y) * texel_v, 0.f, 0.f},
          ml::vec4::zero(),
          ml::vec4::zero(),
        };
        c11 = receiver_depth - depth_bias <= shadow_map->sample_at(uv_varying).x ? 1.f : 0.f;
    }

    // Bilinearly blend the 4 comparison results
    const float c0 = ml::lerp(frac.x, c00, c10);
    const float c1 = ml::lerp(frac.x, c01, c11);
    return ml::lerp(frac.y, c0, c1);
}

inline float sample_shadow_map(
  std::span<const swr::uniform> uniforms,
  std::span<swr::sampler_2d* const> samplers,
  const swr::varying& shadow_clip_position,
  float depth_bias,
  int pcf_mode)
{
    if(uniforms[shadow_map_enabled_uniform_index].i == 0
       || samplers.size() <= shadow_map_sampler_unit
       || samplers[shadow_map_sampler_unit] == nullptr)
    {
        return 1.f;
    }

    const swr::sampler_2d* shadow_map = samplers[shadow_map_sampler_unit];
    const ml::tvec2<int> shadow_map_size = shadow_map->size(0);

    const ml::vec4 clip = shadow_clip_position.value;
    if(std::abs(clip.w) <= 0.0001f)
    {
        return 1.f;
    }

    const ml::vec3 ndc = clip.xyz() / clip.w;
    ml::vec2 uv{
      ndc.x * 0.5f + 0.5f,
      ndc.y * 0.5f + 0.5f,
    };
    float receiver_depth = ndc.z * 0.5f + 0.5f;

    // Guard against tiny precision spillover at shadow-map borders.
    const float u_guard = 1.f / shadow_map_size.x;
    const float v_guard = 1.f / shadow_map_size.y;
    constexpr float depth_guard = 1.f / 4096.f;

    if(uv.x < -u_guard || uv.x > 1.f + u_guard
       || uv.y < -v_guard || uv.y > 1.f + v_guard
       || receiver_depth < -depth_guard || receiver_depth > 1.f + depth_guard)
    {
        return 1.f;
    }

    uv.x = std::clamp(uv.x, 0.f, 1.f);
    uv.y = std::clamp(uv.y, 0.f, 1.f);
    receiver_depth = std::clamp(receiver_depth, 0.f, 1.f);

    if(pcf_mode == 0)
    {
        // No PCF: Single nearest-neighbor sample
        const swr::varying uv_varying{
          ml::vec4{uv.x, uv.y, 0.f, 0.f},
          ml::vec4::zero(),
          ml::vec4::zero(),
        };
        const float stored_depth =
          samplers[shadow_map_sampler_unit]->sample_at(uv_varying).x;
        return receiver_depth - depth_bias <= stored_depth ? 1.f : 0.f;
    }
    else if(pcf_mode == 2)
    {
        // PCF with bilinear comparison (sampler2DShadow-like)
        return shadow_compare_bilinear(shadow_map, uv, receiver_depth, depth_bias);
    }

    // pcf_mode == 1: 3x3 PCF with nearest comparisons
    const float texel_size_u = 1.f / shadow_map_size.x;
    const float texel_size_v = 1.f / shadow_map_size.y;

    float shadow_sum = 0.f;
    for(int dy = -1; dy <= 1; ++dy)
    {
        for(int dx = -1; dx <= 1; ++dx)
        {
            const ml::vec2 sample_uv{
              uv.x + dx * texel_size_u,
              uv.y + dy * texel_size_v,
            };

            // Clamp sample to valid range
            const ml::vec2 clamped_uv{
              std::clamp(sample_uv.x, 0.f, 1.f),
              std::clamp(sample_uv.y, 0.f, 1.f),
            };

            const swr::varying sample_uv_varying{
              ml::vec4{clamped_uv.x, clamped_uv.y, 0.f, 0.f},
              ml::vec4::zero(),
              ml::vec4::zero(),
            };
            const float stored_depth =
              samplers[shadow_map_sampler_unit]->sample_at(sample_uv_varying).x;

            // Compare: in shadow if receiver_depth > stored_depth
            shadow_sum += receiver_depth - depth_bias <= stored_depth ? 1.f : 0.f;
        }
    }

    // Average the 9 samples
    return shadow_sum / 9.f;
}

inline float accumulate_directional_light(
  const ml::vec3& normal,
  int light_count,
  const std::array<ml::vec4, max_lights>& lights)
{
    float light = 0.f;
    const int active_light_count = clamp_light_count(light_count);
    for(int light_index = 0; light_index < active_light_count; ++light_index)
    {
        const ml::vec4& directional_light =
          lights[static_cast<std::size_t>(light_index)];
        light +=
          std::max(ml::dot(normal, directional_light.xyz()), 0.f)
          * directional_light.w;
    }
    return std::clamp(light, 0.f, 1.f);
}

struct LightContribution
{
    ml::vec4 diffuse{0.f, 0.f, 0.f, 0.f};
    ml::vec4 specular{0.f, 0.f, 0.f, 0.f};
};

namespace phong
{

inline LightContribution accumulate_spot_lights(
  const ml::vec3& normal,
  const ml::vec3& position_cameraspace,
  const ml::vec3& view_dir,
  int light_count,
  const std::array<SpotLightData, max_spot_lights>& lights,
  float shininess)
{
    LightContribution contribution{};
    const int active_light_count = clamp_spot_light_count(light_count);
    for(int light_index = 0; light_index < active_light_count; ++light_index)
    {
        const SpotLightData& spot_light =
          lights[static_cast<std::size_t>(light_index)];
        const ml::vec3 light_position = spot_light.position_and_range.xyz();
        const float light_range = spot_light.position_and_range.w;
        if(light_range <= 0.f)
        {
            continue;
        }

        const ml::vec3 spot_direction =
          spot_light.direction_and_brightness.xyz();
        const float brightness = spot_light.direction_and_brightness.w;
        const float inner_cone_cosine = spot_light.params.x;
        const float outer_cone_cosine = spot_light.params.y;
        const ml::vec4 light_color = spot_light.color;

        const ml::vec3 to_light = light_position - position_cameraspace;
        const float distance = to_light.length();
        if(distance <= 0.0001f || distance > light_range)
        {
            continue;
        }

        const ml::vec3 light_dir = to_light / distance;
        const ml::vec3 light_to_fragment = -light_dir;
        const float cone = ml::dot(light_to_fragment, spot_direction);
        if(cone < outer_cone_cosine)
        {
            continue;
        }

        const float attenuation =
          std::pow(std::clamp(1.f - distance / light_range, 0.f, 1.f), 2.f);
        const float cone_falloff =
          std::clamp(
            (cone - outer_cone_cosine)
              / (inner_cone_cosine - outer_cone_cosine + 0.0001f),
            0.f,
            1.f);
        const float intensity =
          brightness * attenuation * cone_falloff;

        const float diffuse =
          std::max(ml::dot(normal, light_dir), 0.f) * intensity;
        contribution.diffuse += light_color * diffuse;

        if(diffuse > 0.f)
        {
            const ml::vec3 reflect_dir =
              (-light_dir + normal * (2.f * ml::dot(normal, light_dir))).normalized();
            contribution.specular +=
              light_color
              * (std::pow(std::max(ml::dot(reflect_dir, view_dir), 0.f), shininess)
                 * intensity);
        }
    }

    contribution.diffuse = ml::clamp_to_unit_interval(contribution.diffuse);
    contribution.specular = ml::clamp_to_unit_interval(contribution.specular);
    return contribution;
}

}    // namespace phong

namespace lit
{

inline LightContribution accumulate_spot_lights(
  const ml::vec3& normal,
  const ml::vec3& position_cameraspace,
  const ml::vec3& view_dir,
  int light_count,
  const std::array<SpotLightData, max_spot_lights>& lights,
  float shininess)
{
    LightContribution contribution{};
    const int active_light_count = clamp_spot_light_count(light_count);

    const float normalized_specular_scale =
      (shininess + 8.f) / (8.f * M_PI);

    for(int light_index = 0; light_index < active_light_count; ++light_index)
    {
        const SpotLightData& spot_light =
          lights[static_cast<std::size_t>(light_index)];

        const ml::vec3 light_position = spot_light.position_and_range.xyz();
        const float light_range = spot_light.position_and_range.w;
        if(light_range <= 0.f)
        {
            continue;
        }

        const ml::vec3 spot_direction =
          spot_light.direction_and_brightness.xyz().normalized();
        const float brightness = spot_light.direction_and_brightness.w;
        const float inner_cone_cosine = spot_light.params.x;
        const float outer_cone_cosine = spot_light.params.y;
        const ml::vec4 light_color = spot_light.color;

        const ml::vec3 to_light = light_position - position_cameraspace;
        const float distance = to_light.length();
        if(distance <= 0.0001f || distance > light_range)
        {
            continue;
        }

        const ml::vec3 light_dir = to_light / distance;

        const ml::vec3 light_to_fragment = -light_dir;
        const float cone = ml::dot(light_to_fragment, spot_direction);
        if(cone < outer_cone_cosine)
        {
            continue;
        }

        const float attenuation =
          std::pow(std::clamp(1.f - distance / light_range, 0.f, 1.f), 2.f);

        const float cone_falloff =
          std::clamp(
            (cone - outer_cone_cosine)
              / (inner_cone_cosine - outer_cone_cosine + 0.0001f),
            0.f,
            1.f);

        const float intensity =
          brightness * attenuation * cone_falloff;

        const float NoL =
          std::max(ml::dot(normal, light_dir), 0.f);

        contribution.diffuse +=
          light_color * (NoL * intensity);

        if(NoL > 0.f)
        {
            const ml::vec3 half_dir =
              (light_dir + view_dir).normalized();

            const float NoH =
              std::max(ml::dot(normal, half_dir), 0.f);

            const float specular =
              normalized_specular_scale
              * std::pow(NoH, shininess)
              * NoL
              * intensity;

            contribution.specular +=
              light_color * specular;
        }
    }

    contribution.diffuse = ml::clamp_to_unit_interval(contribution.diffuse);
    contribution.specular = ml::clamp_to_unit_interval(contribution.specular);

    return contribution;
}

}    // namespace lit

class ShadowDepth : public swr::program<ShadowDepth>
{
public:
    ShadowDepth() = default;

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
        iqs = {};
    }

    void vertex_shader(
      [[maybe_unused]] int gl_VertexID,
      [[maybe_unused]] int gl_InstanceID,
      std::span<const ml::vec4> attribs,
      ml::vec4& gl_Position,
      [[maybe_unused]] float& gl_PointSize,
      [[maybe_unused]] std::span<float> gl_ClipDistance,
      [[maybe_unused]] std::span<ml::vec4> varyings) const override
    {
        const ml::mat4x4 proj = uniforms[camera_projection_uniform_index].m4;
        const ml::mat4x4 view = uniforms[camera_view_uniform_index].m4;
        gl_Position = proj * view * attribs[0];
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      [[maybe_unused]] std::span<const swr::varying> varyings,
      float& gl_FragDepth,
      ml::vec4& gl_FragColor) const override
    {
        gl_FragColor = {gl_FragDepth, gl_FragDepth, gl_FragDepth, 1.f};
        return swr::accept;
    }
};

/**
 * A shader that applies coloring and directional lighting.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *   attribute 1: vertex normal
 *
 * varyings:
 *   location 0: lit color
 *
 * uniforms:
 *   location camera_projection_uniform_index: projection matrix [mat4x4]
 *   location camera_view_uniform_index: view matrix [mat4x4]
 *   location directional_light_count_uniform_index: directional light count [int]
 *   location directional_light_uniform_base_index..
 *            (directional_light_uniform_base_index + max_lights - 1):
 *            directional lights in camera space, brightness in w [vec4]
 *   location spot_light_count_uniform_index: spot light count [int]
 *   location spot_light_uniform_base_index..: spot light tuples:
 *      position/range, direction/brightness, params(inner/outer cone) [vec4]
 *
 */

class ColorFlat : public swr::program<ColorFlat>
{
public:
    ColorFlat() = default;

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
          swr::interpolation_qualifier::flat};
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
        const int light_count = uniforms[directional_light_count_uniform_index].i;
        const auto lights = load_directional_lights(uniforms);
        const int spot_light_count = uniforms[spot_light_count_uniform_index].i;
        const auto spot_lights = load_spot_lights(uniforms);
        const ml::vec4 diffuse_color = uniforms[material_color_uniform_index].v4;
        const ml::vec4 ambient_color = diffuse_color;

        // transform vertex.
        const ml::vec4 pos_cam = view * attribs[0];
        gl_Position = proj * pos_cam;

        auto n = ml::vec4((view * attribs[1]).xyz(), 0).normalized(); /* normal in camera space */
        auto l = accumulate_directional_light(
          n.xyz(),
          light_count,
          lights);
        const LightContribution spot = phong::accumulate_spot_lights(
          n.xyz(),
          pos_cam.xyz(),
          ml::vec3{0.f, 0.f, 1.f},
          spot_light_count,
          spot_lights,
          1.f);

        varyings[0] =
          ml::clamp_to_unit_interval(ambient_color * 0.2f + diffuse_color * l + diffuse_color * spot.diffuse);
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      ml::vec4& gl_FragColor) const override
    {
        // write color.
        gl_FragColor = varyings[0];

        // accept fragment.
        return swr::accept;
    }
};

class ColorSmooth : public swr::program<ColorSmooth>
{
public:
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
            light_index < max_lights;
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
        const std::array<ml::vec4, max_lights> lights = {
          direction0,
          direction1};
        const auto spot_lights = load_spot_lights(uniforms);

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
          spot_lights,
          1.f);

        // write color.
        gl_FragColor =
          ml::clamp_to_unit_interval(ambient_color * 0.2f + diffuse_color * l + diffuse_color * spot.diffuse);

        // accept fragment.
        return swr::accept;
    }
};

/**
 * A Phong shader with two directional lights.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *   attribute 1: vertex normal
 *
 * varyings:
 *   location 0: normal in camera space         [smooth]
 *
 * uniforms:
 *   location camera_projection_uniform_index: projection matrix [mat4x4]
 *   location camera_view_uniform_index: view matrix [mat4x4]
 *   location directional_light_count_uniform_index: directional light count [int]
 *   location directional_light_uniform_base_index..
 *            (directional_light_uniform_base_index + max_lights - 1):
 *            directional lights in camera space, brightness in w [vec4]
 *   location spot_light_count_uniform_index: spot light count [int]
 *   location spot_light_uniform_base_index..: spot light tuples:
 *      position/range, direction/brightness, params(inner/outer cone) [vec4]
 *
 */

class PhongSmooth : public swr::program<PhongSmooth>
{
    static constexpr float ambient_strength = 0.15f;
    static constexpr float specular_strength = 0.5f;
    static constexpr float shininess = 64.f;

public:
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
        const auto spot_lights = load_spot_lights(uniforms);
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
          spot_lights,
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

class LitSmooth : public swr::program<LitSmooth>
{
    static constexpr float ambient_strength = 0.15f;

    // Dielectric-ish default reflectance.
    static constexpr float specular_strength = 0.04f;

    static constexpr float shininess = 64.f;
    static constexpr float normalized_specular_scale =
      (shininess + 8.f) / (8.f * M_PI);

public:
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
        const auto lights = load_directional_lights(uniforms);

        const int spot_light_count =
          clamp_spot_light_count(uniforms[spot_light_count_uniform_index].i);
        const auto spot_lights = load_spot_lights(uniforms);
        const float shadow =
          spot_light_count > 0
            ? sample_shadow_map(
                uniforms,
                samplers,
                varyings[2],
                uniforms[shadow_map_params_uniform_index].v4.x,
                static_cast<int>(uniforms[shadow_map_params_uniform_index].v4.y + 0.5f))
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
            const ml::vec4& directional_light =
              lights[static_cast<std::size_t>(light_index)];

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
          spot_lights,
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

class ColorOnly : public swr::program<ColorOnly>
{
public:
    ColorOnly() = default;

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
        iqs.clear();
    }

    void vertex_shader(
      [[maybe_unused]] int gl_VertexID,
      [[maybe_unused]] int gl_InstanceID,
      std::span<const ml::vec4> attribs,
      ml::vec4& gl_Position,
      [[maybe_unused]] float& gl_PointSize,
      [[maybe_unused]] std::span<float> gl_ClipDistance,
      [[maybe_unused]] std::span<ml::vec4> varyings) const override
    {
        ml::mat4x4 proj = uniforms[camera_projection_uniform_index].m4;
        ml::mat4x4 view = uniforms[camera_view_uniform_index].m4;

        // transform vertex.
        gl_Position = proj * view * attribs[0];
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      [[maybe_unused]] std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      ml::vec4& gl_FragColor) const override
    {
        // write color.
        gl_FragColor = uniforms[material_color_uniform_index].v4;

        // accept fragment.
        return swr::accept;
    }
};

class ShadowMapDebug : public swr::program<ShadowMapDebug>
{
public:
    ShadowMapDebug() = default;

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
          swr::interpolation_qualifier::smooth};
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
        gl_Position = attribs[0];
        varyings[0] = attribs[2];
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      ml::vec4& gl_FragColor) const override
    {
        if(samplers.size() <= shadow_map_sampler_unit
           || samplers[shadow_map_sampler_unit] == nullptr)
        {
            gl_FragColor = {0.f, 0.f, 0.f, 1.f};
            return swr::accept;
        }

        const float depth =
          std::clamp(
            samplers[shadow_map_sampler_unit]->sample_at(varyings[0]).x,
            0.f,
            1.f);

        // Raw depth is usually heavily concentrated near 1.0 in perspective
        // shadow maps; remap it to reveal structure in the debug view.
        const float depth_debug =
          std::pow(std::max(1.f - depth, 0.f), 0.2f);

        gl_FragColor = {depth_debug, depth_debug, depth_debug, 1.f};
        return swr::accept;
    }
};

class TexturedShinyFloor : public swr::program<TexturedShinyFloor>
{
    const ml::vec4 light_color{1.f, 1.f, 1.f, 1.f};
    const ml::vec4 light_specular_color{1.f, 1.f, 1.f, 1.f};
    static constexpr float ambient_diffuse_factor = 0.10f;
    static constexpr float specular_strength = 0.35f;
    static constexpr float shininess = 24.f;

public:
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
        const ml::vec4 normal = varyings[2];
        const ml::vec4 tangent = varyings[3];
        const ml::vec4 bitangent = varyings[4];
        const ml::vec4 eye_direction = varyings[5];
        const float shadow = sample_shadow_map(
          uniforms,
          samplers,
          varyings[6],
          uniforms[shadow_map_params_uniform_index].v4.x,
          static_cast<int>(uniforms[shadow_map_params_uniform_index].v4.y + 0.5f));

        const ml::vec4 base_color = samplers[0]->sample_at(tex_coords);
        const ml::vec3 material_normal =
          (samplers[1]->sample_at(tex_coords) * 2.f - 1.f).xyz();

        const ml::mat4x4 tbn = ml::mat4x4{
          tangent,
          bitangent,
          normal,
          ml::vec4::zero()}
                                 .transposed();
        const ml::vec3 N =
          (tbn * ml::vec4{material_normal, 0.f}).xyz().normalized();
        const ml::vec3 view_dir = eye_direction.xyz().normalized();

        const int light_count = clamp_light_count(
          uniforms[directional_light_count_uniform_index].i);
        const auto lights = load_directional_lights(uniforms);
        const int spot_light_count = clamp_spot_light_count(
          uniforms[spot_light_count_uniform_index].i);
        const auto spot_lights = load_spot_lights(uniforms);

        float lambertian_sum = 0.f;
        float specular_sum = 0.f;
        for(int light_index = 0; light_index < light_count; ++light_index)
        {
            const ml::vec4& directional_light =
              lights[static_cast<std::size_t>(light_index)];
            const ml::vec3 light_dir = directional_light.xyz().normalized();
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
          ml::vec4(varyings[1]).xyz(),
          view_dir,
          spot_light_count,
          spot_lights,
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

class TexturedFloor : public swr::program<TexturedFloor>
{
    const ml::vec4 light_color{1.f, 1.f, 1.f, 1.f};
    const ml::vec4 light_specular_color{1.f, 1.f, 1.f, 1.f};

    static constexpr float ambient_diffuse_factor = 0.10f;
    static constexpr float specular_strength = 0.04f;
    static constexpr float shininess = 24.f;

public:
    TexturedFloor() = default;

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
        const ml::vec4 normal = varyings[2];
        const ml::vec4 tangent = varyings[3];
        const ml::vec4 bitangent = varyings[4];
        const ml::vec4 eye_direction = varyings[5];
        const float shadow = sample_shadow_map(
          uniforms,
          samplers,
          varyings[6],
          uniforms[shadow_map_params_uniform_index].v4.x,
          uniforms[shadow_map_params_uniform_index].v4.y > 0.5f);

        const ml::vec4 base_color = samplers[0]->sample_at(tex_coords);
        const ml::vec3 material_normal =
          (samplers[1]->sample_at(tex_coords) * 2.f - 1.f).xyz();

        const ml::mat4x4 tbn = ml::mat4x4{
          tangent,
          bitangent,
          normal,
          ml::vec4::zero()}
                                 .transposed();
        const ml::vec3 N =
          (tbn * ml::vec4{material_normal, 0.f}).xyz().normalized();
        const ml::vec3 view_dir = eye_direction.xyz().normalized();

        const int light_count = clamp_light_count(
          uniforms[directional_light_count_uniform_index].i);
        const auto lights = load_directional_lights(uniforms);
        const int spot_light_count = clamp_spot_light_count(
          uniforms[spot_light_count_uniform_index].i);
        const auto spot_lights = load_spot_lights(uniforms);

        float lambertian_sum = 0.f;
        float specular_sum = 0.f;

        const float normalized_specular_scale =
          (shininess + 8.f) / (8.f * M_PI);

        for(int light_index = 0; light_index < light_count; ++light_index)
        {
            const ml::vec4& directional_light =
              lights[static_cast<std::size_t>(light_index)];

            const ml::vec3 light_dir = directional_light.xyz().normalized();
            const float brightness = directional_light.w;

            const float NoL =
              std::clamp(ml::dot(N, light_dir), 0.f, 1.f);

            lambertian_sum += NoL * brightness;

            if(NoL > 0.f)
            {
                const ml::vec3 half_dir =
                  (light_dir + view_dir).normalized();

                const float NoH =
                  std::clamp(ml::dot(N, half_dir), 0.f, 1.f);

                specular_sum +=
                  normalized_specular_scale
                  * std::pow(NoH, shininess)
                  * NoL
                  * brightness;
            }
        }

        lambertian_sum = std::clamp(lambertian_sum, 0.f, 1.f);
        specular_sum = std::clamp(specular_sum, 0.f, 1.f);
        const LightContribution spot = lit::accumulate_spot_lights(
          N,
          ml::vec4(varyings[1]).xyz(),
          view_dir,
          spot_light_count,
          spot_lights,
          shininess);
        const ml::vec4 shadowed_spot_diffuse = spot.diffuse * shadow;
        const ml::vec4 shadowed_spot_specular = spot.specular * shadow;

        const ml::vec4 diffuse_base =
          base_color * (1.f - specular_strength);

        const ml::vec4 ambient_color =
          diffuse_base * ambient_diffuse_factor;

        const ml::vec4 diffuse_color =
          light_color * diffuse_base * lambertian_sum
          + diffuse_base * shadowed_spot_diffuse;

        const ml::vec4 specular_color =
          light_specular_color * (specular_strength * specular_sum)
          + shadowed_spot_specular * specular_strength;

        gl_FragColor = ml::clamp_to_unit_interval(
          ambient_color + diffuse_color + specular_color);
        gl_FragColor.w = base_color.w;

        return swr::accept;
    }
};

} /* namespace shader */

/**
 * Software Rasterizer Playground.
 *
 * Common shader functionality.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <algorithm>
#include <numbers>

#include <swr/swr.h>
#include <swr/shaders.h>

#include "shader_constants.h"

namespace shader
{

/*
 * Lighting.
 */

inline int clamp_light_count(
  int light_count)
{
    return std::clamp(
      light_count,
      0,
      static_cast<int>(max_directional_lights));
}

inline int clamp_spot_light_count(
  int light_count)
{
    return std::clamp(
      light_count,
      0,
      static_cast<int>(max_spot_lights));
}

inline std::array<ml::vec4, max_directional_lights> load_directional_lights(
  std::span<const swr::uniform> uniforms)
{
    std::array<ml::vec4, max_directional_lights> lights{};
    for(std::size_t light_index = 0; light_index < lights.size(); ++light_index)
    {
        lights[light_index] =
          uniforms[directional_light_uniform_index(light_index)].v4;
    }
    return lights;
}

inline const ml::vec4& get_directional_light(
  std::span<const swr::uniform> uniforms,
  std::size_t light_index)
{
    return uniforms[directional_light_uniform_index(light_index)].v4;
}

inline float accumulate_directional_light(
  const ml::vec3& normal,
  int light_count,
  const std::array<ml::vec4, max_directional_lights>& lights)
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
  std::span<const swr::uniform> uniforms,
  float shininess)
{
    LightContribution contribution{};
    const int active_light_count = clamp_spot_light_count(light_count);
    for(int light_index = 0; light_index < active_light_count; ++light_index)
    {
        const std::size_t uniform_light_index =
          static_cast<std::size_t>(light_index);
        const ml::vec4& position_and_range =
          uniforms[spot_light_uniform_index(
                     uniform_light_index,
                     spot_light_position_uniform_offset)]
            .v4;
        const float light_range = position_and_range.w;
        if(light_range <= 0.f)
        {
            continue;
        }

        const ml::vec4& direction_and_brightness =
          uniforms[spot_light_uniform_index(
                     uniform_light_index,
                     spot_light_direction_uniform_offset)]
            .v4;
        const ml::vec4& params =
          uniforms[spot_light_uniform_index(
                     uniform_light_index,
                     spot_light_params_uniform_offset)]
            .v4;
        const ml::vec4& light_color =
          uniforms[spot_light_uniform_index(
                     uniform_light_index,
                     spot_light_color_uniform_offset)]
            .v4;

        const ml::vec3 light_position = position_and_range.xyz();
        const ml::vec3 spot_direction = direction_and_brightness.xyz();
        const float brightness = direction_and_brightness.w;
        const float inner_cone_cosine = params.x;
        const float outer_cone_cosine = params.y;

        const ml::vec3 to_light = light_position - position_cameraspace;
        const float distance_sq = ml::dot(to_light, to_light);
        const float light_range_sq = light_range * light_range;
        if(distance_sq <= 1.0e-8f || distance_sq > light_range_sq)
        {
            continue;
        }

        const float inv_distance = 1.f / std::sqrt(distance_sq);
        const ml::vec3 light_dir = to_light * inv_distance;
        const ml::vec3 light_to_fragment = -light_dir;
        const float cone = ml::dot(light_to_fragment, spot_direction);
        if(cone < outer_cone_cosine)
        {
            continue;
        }

        const float inv_light_range = 1.f / light_range;
        const float attenuation =
          std::clamp(1.f - distance_sq * inv_distance * inv_light_range, 0.f, 1.f);
        const float inv_cone_range =
          1.f / (inner_cone_cosine - outer_cone_cosine + 0.0001f);
        const float cone_falloff =
          std::clamp(
            (cone - outer_cone_cosine) * inv_cone_range,
            0.f,
            1.f);
        const float intensity =
          brightness * attenuation * attenuation * cone_falloff;

        const float diffuse =
          std::max(ml::dot(normal, light_dir), 0.f) * intensity;
        contribution.diffuse += light_color * diffuse;

        if(diffuse > 0.f)
        {
            const ml::vec3 reflect_dir =
              -light_dir + normal * (2.f * ml::dot(normal, light_dir));
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
  std::span<const swr::uniform> uniforms,
  float shininess)
{
    LightContribution contribution{};
    const int active_light_count = clamp_spot_light_count(light_count);

    const float normalized_specular_scale =
      (shininess + 8.f) / (8.f * std::numbers::pi_v<float>);

    for(int light_index = 0; light_index < active_light_count; ++light_index)
    {
        const std::size_t uniform_light_index =
          static_cast<std::size_t>(light_index);
        const ml::vec4& position_and_range =
          uniforms[spot_light_uniform_index(
                     uniform_light_index,
                     spot_light_position_uniform_offset)]
            .v4;
        const float light_range = position_and_range.w;
        if(light_range <= 0.f)
        {
            continue;
        }

        const ml::vec4& direction_and_brightness =
          uniforms[spot_light_uniform_index(
                     uniform_light_index,
                     spot_light_direction_uniform_offset)]
            .v4;
        const ml::vec4& params =
          uniforms[spot_light_uniform_index(
                     uniform_light_index,
                     spot_light_params_uniform_offset)]
            .v4;
        const ml::vec4& light_color =
          uniforms[spot_light_uniform_index(
                     uniform_light_index,
                     spot_light_color_uniform_offset)]
            .v4;

        const ml::vec3 light_position = position_and_range.xyz();
        const ml::vec3 spot_direction = direction_and_brightness.xyz();
        const float brightness = direction_and_brightness.w;
        const float inner_cone_cosine = params.x;
        const float outer_cone_cosine = params.y;

        const ml::vec3 to_light = light_position - position_cameraspace;
        const float distance_sq = ml::dot(to_light, to_light);
        const float light_range_sq = light_range * light_range;
        if(distance_sq <= 1.0e-8f || distance_sq > light_range_sq)
        {
            continue;
        }

        const float inv_distance = 1.f / std::sqrt(distance_sq);
        const ml::vec3 light_dir = to_light * inv_distance;

        const ml::vec3 light_to_fragment = -light_dir;
        const float cone = ml::dot(light_to_fragment, spot_direction);
        if(cone < outer_cone_cosine)
        {
            continue;
        }

        const float inv_light_range = 1.f / light_range;
        const float attenuation =
          std::clamp(1.f - distance_sq * inv_distance * inv_light_range, 0.f, 1.f);

        const float inv_cone_range =
          1.f / (inner_cone_cosine - outer_cone_cosine + 0.0001f);
        const float cone_falloff =
          std::clamp(
            (cone - outer_cone_cosine) * inv_cone_range,
            0.f,
            1.f);

        const float intensity =
          brightness * attenuation * attenuation * cone_falloff;

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

/*
 * Shadow map sampling.
 */

inline float legacy_shadow_compare_bilinear(
  const swr::sampler_2d& shadow_map,
  const ml::vec2& uv,
  float receiver_depth,
  float depth_bias)
{
    const ml::tvec2<int> shadow_map_size = shadow_map.size(0);
    const float texel_u = 1.f / shadow_map_size.x;
    const float texel_v = 1.f / shadow_map_size.y;

    const ml::vec2 uv_texel{uv.x / texel_u, uv.y / texel_v};
    const ml::vec2 frac{
      uv_texel.x - std::floor(uv_texel.x),
      uv_texel.y - std::floor(uv_texel.y),
    };

    auto sample_compare = [&shadow_map, receiver_depth, depth_bias](
                            float sample_u,
                            float sample_v) -> float
    {
        const swr::varying uv_varying{
          {sample_u, sample_v, 0.f, 0.f},
          ml::vec4::zero(),
          ml::vec4::zero(),
        };
        return receiver_depth - depth_bias <= shadow_map.sample_at(uv_varying).x ? 1.f : 0.f;
    };

    const float c00 =
      sample_compare(uv.x - frac.x * texel_u, uv.y - frac.y * texel_v);
    const float c10 =
      sample_compare(uv.x + (1.f - frac.x) * texel_u, uv.y - frac.y * texel_v);
    const float c01 =
      sample_compare(uv.x - frac.x * texel_u, uv.y + (1.f - frac.y) * texel_v);
    const float c11 =
      sample_compare(
        uv.x + (1.f - frac.x) * texel_u,
        uv.y + (1.f - frac.y) * texel_v);

    const float c0 = ml::lerp(frac.x, c00, c10);
    const float c1 = ml::lerp(frac.x, c01, c11);
    return ml::lerp(frac.y, c0, c1);
}

inline float shadow_noise_1d(
  const ml::vec2& uv)
{
    const float seed =
      uv.x * 12.9898f
      + uv.y * 78.233f;
    const float noise = std::sin(seed) * 43758.5453f;
    return noise - std::floor(noise);
}

inline ml::vec2 rotate_shadow_offset(
  const ml::vec2& offset,
  float angle)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return {
      offset.x * c - offset.y * s,
      offset.x * s + offset.y * c,
    };
}

inline float shadow_rotation_from_texel(
  int texel_x,
  int texel_y)
{
    const ml::vec2 texel_seed{
      static_cast<float>(texel_x),
      static_cast<float>(texel_y),
    };
    return shadow_noise_1d(texel_seed) * 2.f * std::numbers::pi_v<float>;
}

inline float interleaved_shadow_rotation(
  int texel_x,
  int texel_y)
{
    static constexpr std::array<float, 16> rotation_steps{{
      0.f / 16.f,
      8.f / 16.f,
      2.f / 16.f,
      10.f / 16.f,
      12.f / 16.f,
      4.f / 16.f,
      14.f / 16.f,
      6.f / 16.f,
      3.f / 16.f,
      11.f / 16.f,
      1.f / 16.f,
      9.f / 16.f,
      15.f / 16.f,
      7.f / 16.f,
      13.f / 16.f,
      5.f / 16.f,
    }};

    const int pattern_x = texel_x & 3;
    const int pattern_y = texel_y & 3;
    const float step =
      rotation_steps[static_cast<std::size_t>(pattern_y * 4 + pattern_x)];
    return step * 2.f * std::numbers::pi_v<float>;
}

inline float sample_stochastic_shadow_pattern(
  const swr::sampler_shadow_2d& shadow_map,
  const ml::vec3& projected_shadow_sample,
  const std::array<ml::vec2, 4>& base_offsets,
  const ml::vec2& inv_shadow_map_size,
  float rotation,
  bool include_center_sample)
{
    float shadow_sum = 0.f;
    for(const ml::vec2& base_offset: base_offsets)
    {
        const ml::vec2 offset =
          rotate_shadow_offset(base_offset, rotation);
        shadow_sum += swr::texture(
          shadow_map,
          {
            projected_shadow_sample.x + offset.x * inv_shadow_map_size.x,
            projected_shadow_sample.y + offset.y * inv_shadow_map_size.y,
            projected_shadow_sample.z,
          });
    }

    float sample_count = static_cast<float>(base_offsets.size());
    if(include_center_sample)
    {
        shadow_sum += swr::texture(
          shadow_map,
          projected_shadow_sample);
        sample_count += 1.f;
    }

    return shadow_sum / sample_count;
}

inline float sample_shadow_map(
  std::span<const swr::uniform> uniforms,
  const swr::sampler_2d& legacy_shadow_map,
  const swr::sampler_shadow_2d& shadow_map,
  const swr::varying& shadow_clip_position,
  float depth_bias,
  int pcf_mode)
{
    if(uniforms[shadow_map_enabled_uniform_index].i == 0)
    {
        return 1.f;
    }

    const ml::vec2 inv_shadow_map_size = shadow_map.size_reciprocal(0);

    const ml::vec4 clip = shadow_clip_position.value;
    if(std::abs(clip.w) <= 0.0001f)
    {
        return 1.f;
    }

    const ml::vec3 projected = clip.xyz() / clip.w;

    // Guard against tiny precision spillover at shadow-map borders.
    const float texel_size_u = inv_shadow_map_size.x;
    const float texel_size_v = inv_shadow_map_size.y;
    constexpr float depth_guard = 1.f / 4096.f;

    if(projected.x < -texel_size_u || projected.x > 1.f + texel_size_u
       || projected.y < -texel_size_v || projected.y > 1.f + texel_size_v
       || projected.z < -depth_guard || projected.z > 1.f + depth_guard)
    {
        return 1.f;
    }

    const ml::vec2 uv{
      std::clamp(projected.x, 0.f, 1.f),
      std::clamp(projected.y, 0.f, 1.f),
    };
    const ml::tvec2<int> shadow_map_size = shadow_map.size(0);
    const float receiver_depth =
      std::clamp(projected.z, 0.f, 1.f);
    const ml::vec3 projected_shadow_sample{
      projected.x,
      projected.y,
      projected.z - depth_bias,
    };

    if(pcf_mode == 0)
    {
        return swr::texture(
          shadow_map,
          projected_shadow_sample);
    }
    if(pcf_mode == 1)
    {
        float shadow_sum = 0.f;
        for(int dy = -1; dy <= 1; ++dy)
        {
            for(int dx = -1; dx <= 1; ++dx)
            {
                const ml::vec2 sample_uv{
                  std::clamp(uv.x + dx * texel_size_u, 0.f, 1.f),
                  std::clamp(uv.y + dy * texel_size_v, 0.f, 1.f),
                };
                const swr::varying sample_uv_varying{
                  {sample_uv.x, sample_uv.y, 0.f, 0.f},
                  ml::vec4::zero(),
                  ml::vec4::zero(),
                };
                const float stored_depth =
                  legacy_shadow_map.sample_at(sample_uv_varying).x;
                shadow_sum += receiver_depth - depth_bias <= stored_depth ? 1.f : 0.f;
            }
        }

        return shadow_sum / 9.f;
    }
    if(pcf_mode == 2)
    {
        return legacy_shadow_compare_bilinear(
          legacy_shadow_map,
          uv,
          receiver_depth,
          depth_bias);
    }
    if(pcf_mode == 3 || pcf_mode == 4)
    {
        float shadow_sum = 0.f;
        for(int dy = -1; dy <= 1; ++dy)
        {
            for(int dx = -1; dx <= 1; ++dx)
            {
                shadow_sum += swr::texture(
                  shadow_map,
                  {
                    projected_shadow_sample.x + dx * texel_size_u,
                    projected_shadow_sample.y + dy * texel_size_v,
                    projected_shadow_sample.z,
                  });
            }
        }

        return shadow_sum / 9.f;
    }
    if(pcf_mode >= 5)
    {
        const std::array<ml::vec2, 4> base_offsets{{
          {-0.85f, -0.20f},
          {0.25f, -0.90f},
          {-0.30f, 0.80f},
          {0.90f, 0.30f},
        }};

        const int texel_x = static_cast<int>(
          std::clamp(
            uv.x * static_cast<float>(std::max(shadow_map_size.x - 1, 0)),
            0.f,
            static_cast<float>(std::max(shadow_map_size.x - 1, 0))));
        const int texel_y = static_cast<int>(
          std::clamp(
            uv.y * static_cast<float>(std::max(shadow_map_size.y - 1, 0)),
            0.f,
            static_cast<float>(std::max(shadow_map_size.y - 1, 0))));

        float rotation = shadow_noise_1d(uv) * 2.f * std::numbers::pi_v<float>;
        bool include_center_sample = false;

        if(pcf_mode == 6)
        {
            include_center_sample = true;
        }
        else if(pcf_mode == 7)
        {
            rotation = shadow_rotation_from_texel(texel_x, texel_y);
        }
        else if(pcf_mode == 8)
        {
            rotation = interleaved_shadow_rotation(texel_x, texel_y);
        }

        return sample_stochastic_shadow_pattern(
          shadow_map,
          projected_shadow_sample,
          base_offsets,
          inv_shadow_map_size,
          rotation,
          include_center_sample);
    }
    return 1.f;
}

}    // namespace shader

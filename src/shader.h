/**
 * Software Rasterizer Playground.
 *
 * color shader with directional lighting.
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

#include "swr/swr.h"
#include "swr/shaders.h"

namespace shader
{

constexpr std::size_t max_lights = 2;

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
        lights[light_index] = uniforms[3 + light_index].v4;
    }
    return lights;
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
 *   location 0: projection matrix              [mat4x4]
 *   location 1: view matrix                    [mat4x4]
 *   location 2: directional light count        [int]
 *   location 3..(3 + max_lights - 1): directional lights in camera space,
 *                                     brightness in w [vec4]
 *
 */

class ColorFlat : public swr::program<ColorFlat>
{
    ml::vec4 diffuse_color{1, 0, 0, 1};
    ml::vec4 ambient_color{1, 0, 0, 1};

public:
    ColorFlat() = default;
    explicit ColorFlat(ml::vec4 in_color)
    : diffuse_color{in_color}
    , ambient_color{in_color}
    {
    }

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
        ml::mat4x4 proj = uniforms[0].m4;
        ml::mat4x4 view = uniforms[1].m4;
        const int light_count = uniforms[2].i;
        const auto lights = load_directional_lights(uniforms);

        // transform vertex.
        const ml::vec4 pos_cam = view * attribs[0];
        gl_Position = proj * pos_cam;

        auto n = ml::vec4((view * attribs[1]).xyz(), 0).normalized(); /* normal in camera space */
        auto l = accumulate_directional_light(
          n.xyz(),
          light_count,
          lights);

        varyings[0] = ambient_color * 0.2f + diffuse_color * l;
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
    ml::vec4 diffuse_color{1, 0, 0, 1};
    ml::vec4 ambient_color{1, 0, 0, 1};

public:
    ColorSmooth() = default;
    explicit ColorSmooth(ml::vec4 in_color)
    : diffuse_color{in_color}
    , ambient_color{in_color}
    {
    }

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
        ml::mat4x4 proj = uniforms[0].m4;
        ml::mat4x4 view = uniforms[1].m4;
        const int light_count = clamp_light_count(uniforms[2].i);
        const auto lights = load_directional_lights(uniforms);

        // transform vertex.
        const ml::vec4 pos_cam = view * attribs[0];
        gl_Position = proj * pos_cam;

        varyings[0] = ml::vec4((view * attribs[1]).xyz(), 0); /* normal in camera space */
        for(int light_index = 0; light_index < light_count; ++light_index)
        {
            varyings[1 + light_index] = lights[static_cast<std::size_t>(light_index)];
        }
        for(std::size_t light_index = static_cast<std::size_t>(light_count);
            light_index < max_lights;
            ++light_index)
        {
            varyings[1 + light_index] = ml::vec4{0.f, 0.f, 0.f, 0.f};
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
        const ml::vec4 normal = varyings[0];
        const ml::vec4 direction0 = varyings[1];
        const ml::vec4 direction1 = varyings[2];
        const int light_count = uniforms[2].i;
        const std::array<ml::vec4, max_lights> lights = {
          direction0,
          direction1};

        const ml::vec3 n = normal.xyz().normalized();
        const float l = accumulate_directional_light(
          n,
          light_count,
          lights);

        // write color.
        gl_FragColor = ambient_color * 0.2f + diffuse_color * l;

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
 *   location 0: projection matrix              [mat4x4]
 *   location 1: view matrix                    [mat4x4]
 *   location 2: directional light count        [int]
 *   location 3..(3 + max_lights - 1): directional lights in camera space,
 *                                     brightness in w [vec4]
 *
 */

class PhongSmooth : public swr::program<PhongSmooth>
{
    ml::vec4 diffuse_color{1, 0, 0, 1};
    ml::vec4 ambient_color{1, 0, 0, 1};

    static constexpr float ambient_strength = 0.15f;
    static constexpr float specular_strength = 0.5f;
    static constexpr float shininess = 64.f;

public:
    PhongSmooth() = default;
    explicit PhongSmooth(ml::vec4 in_color)
    : diffuse_color{in_color}
    , ambient_color{in_color}
    {
    }

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
        ml::mat4x4 proj = uniforms[0].m4;
        ml::mat4x4 view = uniforms[1].m4;
        gl_Position = proj * view * attribs[0];

        varyings[0] = ml::vec4((view * attribs[1]).xyz(), 0.f); /* normal in camera space */
    }

    swr::fragment_shader_result fragment_shader(
      [[maybe_unused]] const ml::vec4& gl_FragCoord,
      [[maybe_unused]] bool gl_FrontFacing,
      [[maybe_unused]] const ml::vec2& gl_PointCoord,
      std::span<const swr::varying> varyings,
      [[maybe_unused]] float& gl_FragDepth,
      ml::vec4& gl_FragColor) const override
    {
        const ml::vec3 N = ml::vec4(varyings[0]).xyz().normalized();
        const int light_count = uniforms[2].i;
        const auto lights = load_directional_lights(uniforms);

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

        const ml::vec3 view_dir = {0.f, 0.f, 1.f};
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

        const ml::vec4 ambient = ambient_color * ambient_strength;
        const ml::vec4 diffuse = diffuse_color * diff;
        const ml::vec4 specular = ml::vec4{1.f, 1.f, 1.f, 0.f} * (specular_strength * spec);

        gl_FragColor = ml::vec4{
          std::min(ambient.x + diffuse.x + specular.x, 1.f),
          std::min(ambient.y + diffuse.y + specular.y, 1.f),
          std::min(ambient.z + diffuse.z + specular.z, 1.f),
          1.f};

        return swr::accept;
    }
};

class ColorOnly : public swr::program<ColorOnly>
{
    ml::vec4 color{1, 0, 0, 1};

public:
    ColorOnly() = default;
    explicit ColorOnly(ml::vec4 in_color)
    : color{in_color}
    {
    }

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
        ml::mat4x4 proj = uniforms[0].m4;
        ml::mat4x4 view = uniforms[1].m4;

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
        gl_FragColor = color;

        // accept fragment.
        return swr::accept;
    }
};

class TexturedFloor : public swr::program<TexturedFloor>
{
    const ml::vec4 light_color{1.f, 1.f, 1.f, 1.f};
    const ml::vec4 light_specular_color{1.f, 1.f, 1.f, 1.f};
    static constexpr float ambient_diffuse_factor = 0.10f;
    static constexpr float specular_strength = 0.35f;
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
        const ml::mat4x4 proj = uniforms[0].m4;
        const ml::mat4x4 view = uniforms[1].m4;
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

        const int light_count = clamp_light_count(uniforms[2].i);
        const auto lights = load_directional_lights(uniforms);

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

        const ml::vec4 ambient_color = base_color * ambient_diffuse_factor;
        const ml::vec4 diffuse_color =
          light_color * base_color * lambertian_sum;
        const ml::vec4 specular_color =
          light_specular_color * (specular_strength * specular_sum);

        gl_FragColor = ml::vec4{
          std::min(ambient_color.x + diffuse_color.x + specular_color.x, 1.f),
          std::min(ambient_color.y + diffuse_color.y + specular_color.y, 1.f),
          std::min(ambient_color.z + diffuse_color.z + specular_color.z, 1.f),
          base_color.w};
        return swr::accept;
    }
};

} /* namespace shader */

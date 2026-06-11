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

#include "swr/swr.h"
#include "swr/shaders.h"

namespace shader
{

/**
 * A shader that applies coloring and directional lighting.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *   attribute 1: vertex normal
 *
 * varyings:
 *   location 0: normal
 *   location 1: light direction in camera space
 *
 * uniforms:
 *   location 0: projection matrix              [mat4x4]
 *   location 1: view matrix                    [mat4x4]
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
        const ml::vec3 light_pos = uniforms[2].v4.xyz();

        // transform vertex.
        const ml::vec4 pos_cam = view * attribs[0];
        gl_Position = proj * pos_cam;

        auto n = ml::vec4((view * attribs[1]).xyz(), 0).normalized();   /* normal in camera space */
        auto d = ml::vec4((light_pos - pos_cam.xyz()).normalized(), 0); /* light direction in camera space */
        auto l = std::clamp(ml::dot(n, d), 0.f, 1.f);

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
          swr::interpolation_qualifier::flat,   /* light direction */
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
        const ml::vec3 light_pos = uniforms[2].v4.xyz();

        // transform vertex.
        const ml::vec4 pos_cam = view * attribs[0];
        gl_Position = proj * pos_cam;

        varyings[0] = ml::vec4((view * attribs[1]).xyz(), 0);                /* normal in camera space */
        varyings[1] = ml::vec4((light_pos - pos_cam.xyz()).normalized(), 0); /* light direction in camera space */
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
        const ml::vec4 direction = varyings[1];

        const ml::vec3 n = normal.xyz().normalized();
        const ml::vec3 d = direction.xyz();

        auto l = std::clamp(ml::dot(n, d), 0.f, 1.f);

        // write color.
        gl_FragColor = ambient_color * 0.2f + diffuse_color * l;

        // accept fragment.
        return swr::accept;
    }
};

/**
 * A Phong shader with a point light.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *   attribute 1: vertex normal
 *
 * varyings:
 *   location 0: normal in camera space         [smooth]
 *   location 1: fragment position in camera space [smooth]
 *
 * uniforms:
 *   location 0: projection matrix              [mat4x4]
 *   location 1: view matrix                    [mat4x4]
 *   location 2: light position in camera space [vec4]
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
          swr::interpolation_qualifier::smooth, /* fragment position */
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
        const ml::vec4 pos_cam = view * attribs[0];

        gl_Position = proj * pos_cam;

        varyings[0] = ml::vec4((view * attribs[1]).xyz(), 0.f); /* normal in camera space */
        varyings[1] = ml::vec4(pos_cam.xyz(), 0.f);             /* position in camera space */
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
        const ml::vec3 frag_pos = ml::vec4(varyings[1]).xyz();
        const ml::vec3 light_pos = uniforms[2].v4.xyz();

        const ml::vec3 L = (light_pos - frag_pos).normalized();
        const ml::vec3 V = (-frag_pos).normalized();
        /* reflect(-L, N) = -L + 2*(N·L)*N */
        const ml::vec3 R = (-L + N * (2.f * ml::dot(N, L))).normalized();

        const float diff = std::max(ml::dot(N, L), 0.f);
        const float spec = std::pow(std::max(ml::dot(R, V), 0.f), shininess);

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

} /* namespace shader */

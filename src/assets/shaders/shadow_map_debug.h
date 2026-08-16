/**
 * Software Rasterizer Playground.
 *
 * Write exponentiated depth component from shadow sampler to color
 * channels as grayscale.
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
 * Write exponentiated depth component from shadow sampler to color channels as grayscale.
 *
 * vertex shader input:
 *   attribute 0: clip-space position (passed through to gl_Position)
 *   attribute 2: texture coordinates for shadow sampler
 *
 * varyings:
 *   location 0: uv for shadow map lookup [smooth]
 *
 * uniforms / samplers:
 *   sampler unit `shadow_map_sampler_unit`: shadow map (sampler2D)
 *
 * Notes:
 *   Samples the shadow map, remaps the depth for debug visualization,
 *   and writes a grayscale value to RGB.
 */
class ShadowMapDebug final
: public swr::program<ShadowMapDebug>
{
public:
    static constexpr std::string_view name = "ShadowMapDepth";

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
        const float depth =
          std::clamp(
            sampler2D(shadow_map_sampler_unit).sample_at(varyings[0]).x,
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

}    // namespace shader
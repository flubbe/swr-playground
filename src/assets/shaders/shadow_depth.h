/**
 * Software Rasterizer Playground.
 *
 * Write linear depth component to color channels as grayscale.
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
 * Write linear depth component to color channels as grayscale.
 *
 * vertex shader input:
 *   attribute 0: vertex position
 *
 * varyings:
 *   none.
 *
 * uniforms:
 *   location `camera_projection_uniform_index`: projection matrix [mat4x4]
 *   location `camera_view_uniform_index`: view matrix [mat4x4]
 *
 * Notes:
 *   The fragment shader writes `gl_FragDepth` into RGB channels for use
 *   with shadow-map or depth-visualization render targets.
 */
class ShadowDepth final
: public swr::program<ShadowDepth>
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

}    // namespace shader

/**
 * Software Rasterizer Playground.
 *
 * Render-facing shared types.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <compare>
#include <cstdint>
#include <functional>

#include "ml/all.h"

template<typename Tag>
struct RenderHandle
{
    std::uint32_t value{0};

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return value != 0;
    }

    auto operator<=>(const RenderHandle&) const = default;
};

struct MeshHandleTag;
struct MaterialHandleTag;
struct ShadowMapHandleTag;

using MeshHandle = RenderHandle<MeshHandleTag>;
using MaterialHandle = RenderHandle<MaterialHandleTag>;
using ShadowMapHandle = RenderHandle<ShadowMapHandleTag>;

/** Part of a mesh using one material. */
struct MeshSection
{
    /** Mesh handle. */
    MeshHandle mesh_handle{};

    /** Material handle. */
    MaterialHandle material_handle{};

    /** Base color used by the lighting shader. */
    ml::vec4 color{1.f, 1.f, 1.f, 1.f};
};

/** One shadow map input for a draw call. */
struct ShadowMapBinding
{
    bool enabled{false};
    ShadowMapHandle handle{};
    ml::mat4x4 clip_from_mesh{ml::mat4x4::identity()};
    float depth_bias{0.0015f};
    bool linear_filter{false};
};

namespace std
{

template<typename Tag>
struct hash<RenderHandle<Tag>>
{
    [[nodiscard]]
    std::size_t operator()(RenderHandle<Tag> handle) const noexcept
    {
        return std::hash<std::uint32_t>{}(handle.value);
    }
};

}    // namespace std

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

#include <ml/all.h>

#include "containers/string.h"

template<typename Tag>
struct RenderHandle
{
    std::uint32_t value{0};

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return value != 0;
    }

    auto operator==(const RenderHandle& other) const
    {
        return value == other.value;
    }

    bool operator==(std::uint32_t other) const
    {
        return value == other;
    }

    RenderHandle& operator++()
    {
        ++value;
        return *this;
    }
    RenderHandle& operator++(int)
    {
        RenderHandle temp = *this;
        ++value;
        return temp;
    }
};

using FrameBufferHandle = RenderHandle<struct FrameBufferHandleTag>;
using MaterialHandle = RenderHandle<struct MaterialHandleTag>;
using MeshHandle = RenderHandle<struct MeshHandleTag>;
using NormalBufferHandle = RenderHandle<struct NormalBufferTag>;
using ShaderHandle = RenderHandle<struct ShaderHandleTag>;
using ShadowMapHandle = RenderHandle<struct ShadowMapHandleTag>;
using TexCoordBufferHandle = RenderHandle<struct TexCoordBufferTag>;
using TextureHandle = RenderHandle<struct TextureHandleTag>;
using VertexBufferHandle = RenderHandle<struct VertexBufferTag>;
using IndexBufferHandle = RenderHandle<struct IndexBufferTag>;

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

/** One shadow map input for a draw call. */
struct ShadowMapBinding
{
    bool enabled{false};
    ShadowMapHandle handle{};
    ml::mat4x4 clip_from_mesh{ml::mat4x4::identity()};
    float depth_bias{0.0015f};
    bool linear_filter{false};
};

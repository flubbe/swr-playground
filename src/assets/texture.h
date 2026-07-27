/**
 * Software Rasterizer Playground.
 *
 * Texture asset loading helpers.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <filesystem>
#include <cstdint>
#include <vector>

namespace assets
{

enum class NormalMapConvention
{
    OpenGL,
    DirectX,
};

struct ImageRGBA8
{
    int width{0};
    int height{0};
    std::vector<std::uint8_t> pixels;
};

[[nodiscard]]
ImageRGBA8 load_texture_rgba8(
  const std::filesystem::path& path);

[[nodiscard]]
ImageRGBA8 load_normal_map_rgba8(
  const std::filesystem::path& path,
  NormalMapConvention convention);

}    // namespace assets

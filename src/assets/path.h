/**
 * Software Rasterizer Playground.
 *
 * An asset path.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <filesystem>
#include <string_view>
#include <utility>

namespace assets
{

/** An asset path. */
struct AssetPath
{
    /** Path identifying the asset. */
    std::filesystem::path path;

    AssetPath() = default;

    AssetPath(const std::filesystem::path& other)
    : path{other}
    {
    }
    AssetPath(std::filesystem::path&& other)
    : path{std::move(other)}
    {
    }

    AssetPath& operator=(
      const std::filesystem::path& other)
    {
        path = other;
        return *this;
    }

    AssetPath& operator=(
      std::filesystem::path&& other)
    {
        path = std::move(other);
        return *this;
    }

    bool operator==(const AssetPath&) const = default;
};

}    // namespace assets

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

#include "reflection/property.h"

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

/*
 * Reflection support.
 */

namespace reflect
{

template<>
struct UnwrapType<assets::AssetPath>
{
    using ValueType = decltype(assets::AssetPath::path);

    static ValueType& get(assets::AssetPath& value) noexcept
    {
        return value.path;
    }
};

}    // namespace reflect

/*
 * Hashing.
 */

namespace std
{

template<>
struct hash<assets::AssetPath>
{
    [[nodiscard]]
    std::size_t operator()(const assets::AssetPath& path) const noexcept
    {
        return std::hash<std::filesystem::path>{}(path.path);
    }
};

}    // namespace std

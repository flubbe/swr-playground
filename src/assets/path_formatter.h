/**
 * Software Rasterizer Playground.
 *
 * std::formatter for assets::AssetPath.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <format>
#include <string_view>

#include "path.h"

namespace std
{

/*
 * Output formatting.
 */

template<>
struct formatter<assets::AssetPath> : formatter<std::string_view>
{
    auto format(
      const assets::AssetPath& asset_path,
      format_context& ctx) const
    {
        return formatter<string_view>::format(
          asset_path.path.string(),
          ctx);
    }
};

}    // namespace std

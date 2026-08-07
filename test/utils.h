/**
 * Software Rasterizer Playground.
 *
 * Test utilities.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <filesystem>
#include <utility>

struct FileCleanup
{
    explicit FileCleanup(std::filesystem::path p)
    : path(std::move(p))
    {
    }

    ~FileCleanup()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::filesystem::path path;
};

/**
 * Software Rasterizer Playground.
 *
 * Resolve an asset to a runtime object.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "assets/path.h"
#include "logging.h"

namespace assets
{

/** Asset resolver. */
struct Resolver
{
    template<typename T>
    T resolve(const assets::AssetPath& path)
    {
        logging::warningf("Resolver::resolve called for '{}'", path.path.string());

        return {};
    }
};

}    // namespace assets

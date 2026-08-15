/**
 * Software Rasterizer Playground.
 *
 * A material that is potentially asynchronously resolved.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "material_manager.h"
#include "resolvable_material.h"

std::optional<MaterialHandle> ResolvableMaterial::try_get()
{
    if(entry)
    {
        return entry->try_get();
    }
    return handle;
}

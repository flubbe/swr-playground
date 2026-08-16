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

bool ResolvableMaterial::is_resolved() const
{
    if(auto* result =
         std::get_if<swr::shared_ptr<MaterialEntry>>(&material))
    {
        return (*result)->is_resolved();
    }

    return std::get<MaterialHandle>(material) != 0;
}

std::optional<MaterialHandle> ResolvableMaterial::try_get() const
{
    if(auto* result =
         std::get_if<swr::shared_ptr<MaterialEntry>>(&material))
    {
        return (*result)->try_get();
    }

    return std::get<MaterialHandle>(material);
}

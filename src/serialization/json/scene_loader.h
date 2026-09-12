/**
 * Software Rasterizer Playground.
 *
 * JSON scene loader.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "serialization/scene_loader.h"
#include "asset_resolver.h"

namespace serial::json
{

/** Load a scene from a JSON description. */
class JsonSceneLoader final
: public SceneLoader
{
    /** Asset resolver reference. */
    AssetResolver& resolver;

public:
    explicit JsonSceneLoader(
      AssetResolver& resolver)
    : resolver{resolver}
    {
    }

    void load(
      Scene& scene,
      std::string_view source_text) override;
};

}    // namespace serial::json

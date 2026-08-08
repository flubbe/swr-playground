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

namespace serial::json
{

/** Load a scene from a JSON description. */
struct JsonSceneLoader final
: public SceneLoader
{
    void load(
      Scene& scene,
      std::string_view source_text) override;
};

}    // namespace serial::json

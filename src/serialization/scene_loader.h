/**
 * Software Rasterizer Playground.
 *
 * Text-based scene loader interface.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <string_view>

/*
 * Forward declarations.
 */

class Scene;

namespace serial
{

/** Load a scene from text. */
struct SceneLoader
{
    /** Virtual destructor. */
    virtual ~SceneLoader() = default;

    /**
     * Load a scene.
     *
     * @note Previous contents of the scene are cleared.
     * @param scene Output scene.
     * @param source_text The text to load the scene from.
     */
    virtual void load(
      Scene& scene,
      std::string_view source_text) = 0;
};

}    // namespace serial

/**
 * Software Rasterizer Playground.
 *
 * Scene update systems.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <string_view>

class Scene;

/** Scene update system interface. */
struct SceneSystem
{
    /** Virtual destructor. */
    virtual ~SceneSystem() = default;

    /** Return the system's name. */
    virtual std::string_view get_name() const = 0;

    /** Run one update tick for the scene. */
    virtual void tick(
      Scene& scene,
      float delta_time) = 0;
};

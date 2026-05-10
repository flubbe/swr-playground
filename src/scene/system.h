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

class Scene;

/** Scene update system interface. */
class SceneSystem
{
public:
    virtual ~SceneSystem() = default;

    /** Run one update tick for the scene. */
    virtual void tick(
      Scene& scene,
      float delta_time) = 0;
};


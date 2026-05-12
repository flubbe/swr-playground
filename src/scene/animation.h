/**
 * Software Rasterizer Playground.
 *
 * Scene animation data.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "ml/all.h"

struct SpinAnimation
{
    ml::vec3 translation{0.f, 0.f, 0.f};
    float angular_speed{1.f};
    float phase_offset{0.f};
};

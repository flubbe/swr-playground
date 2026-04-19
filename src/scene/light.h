/**
 * Software Rasterizer Playground.
 *
 * A simple light.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "ml/all.h"

struct Light
{
    /** light position. */
    ml::vec4 position{5.0f, 5.0f, 10.0f, 0.0f};
};

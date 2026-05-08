/**
 * Software Rasterizer Playground.
 *
 * Camera model.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "ml/all.h"

#include "object.h"

class Camera : public reflect::Reflected<Camera, Object>
{
    /** Vertical field-of-view in radians. */
    float fov_y{static_cast<float>(M_PI) / 8.f};

    /** Near clip plane. */
    float near_plane{5.f};

    /** Far clip plane. */
    float far_plane{60.f};

public:
    static void register_properties(reflect::ClassInfo& class_info);

    Camera() = default;

    ml::mat4x4 get_projection_matrix(float aspect_ratio) const;
};

DECLARE_REFLECTION(Scene, Camera);

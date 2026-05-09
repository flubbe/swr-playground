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

    // FIXME clean up.
    float cached_aspect_ratio{1.f};
    float cached_fov_y{0.f};
    float cached_near_plane{0.f};
    float cached_far_plane{0.f};
    ml::mat4x4 cached_projection{};

public:
    static void register_properties(reflect::ClassInfo& class_info);

    Camera();

    void on_properties_changed() override;
    void update_projection_matrix(float aspect_ratio);
    ml::mat4x4 get_projection_matrix() const;
};

DECLARE_REFLECTION(Scene, Camera);

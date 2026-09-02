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

#include <cstdint>
#include <numbers>

#include <ml/all.h>

#include "object.h"
#include "projection_type.h"

class Camera
: public reflect::Reflected<Camera, Object>
{
    /** Projection mode. */
    ProjectionType projection_type{ProjectionType::Perspective};

    /** Vertical field-of-view in radians. */
    float fov_y{std::numbers::pi_v<float> / 8.f};

    /** Orthographic view height. */
    float orthographic_height{40.f};

    /** Near clip plane. */
    float near_plane{1.f};

    /** Far clip plane. */
    float far_plane{200.f};

    // FIXME clean up.
    float cached_aspect_ratio{1.f};
    ProjectionType cached_projection_type{ProjectionType::Perspective};
    float cached_fov_y{0.f};
    float cached_orthographic_height{0.f};
    float cached_near_plane{0.f};
    float cached_far_plane{0.f};
    ml::mat4x4 cached_projection{};

public:
    static void register_properties(reflect::ClassInfo& class_info);

    Camera();

    void post_load() override;

    void on_properties_changed() override;
    void update_projection_matrix(float aspect_ratio);
    ml::mat4x4 get_projection_matrix() const;

    void set_projection_type(ProjectionType mode);
    ProjectionType get_projection_type() const noexcept;
    void set_orthographic_height(float height);
    float get_orthographic_height() const noexcept;
};

DECLARE_REFLECTION(Scene, Camera);

#pragma once

#include "ml/all.h"

#include "object.h"

class Camera : public Object
{
    /** Sensor width. */
    int width{0};

    /** Sensor height. */
    int height{0};

    /** projection matrix. */
    ml::mat4x4 proj;

protected:
    void update()
    {
        // set projection matrix.
        proj = ml::matrices::perspective_projection(
          static_cast<float>(width) / static_cast<float>(height),
          static_cast<float>(M_PI) / 8,
          5.f,
          60.f);
    }

public:
    Camera() = default;

    Camera(int width, int height)
    : width{width}
    , height{height}
    {
        update();
    }

    const ml::mat4x4& get_projection_matrix() const
    {
        return proj;
    }

    void set_resolution(int in_width, int in_height)
    {
        width = in_width;
        height = in_height;
        update();
    }
};
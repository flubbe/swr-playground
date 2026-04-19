#pragma once

#include "scene/camera.h"

struct DrawParameters
{
    /** Whether to render a wireframe view. */
    bool wireframe{false};

    /** Whether to apply face culling. */
    bool cull_face{true};
};

struct Viewport
{
    Camera camera;
    DrawParameters draw_params;
};

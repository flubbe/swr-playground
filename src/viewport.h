#pragma once

#include <algorithm>

#include "scene/camera.h"
#include "scene/object.h"

enum class ViewportCameraSource
{
    LocalCamera,
    SceneCamera
};

struct DrawParameters
{
    /** Whether to render a wireframe view. */
    bool wireframe{false};

    /** Whether to apply face culling. */
    bool cull_face{true};
};

struct Viewport
{
    Camera local_camera;
    ViewportCameraSource camera_source{ViewportCameraSource::LocalCamera};
    ObjectId scene_camera_id{0};
    bool show_camera_name_overlay{true};
    DrawParameters draw_params;
    int width{1};
    int height{1};

    void set_resolution(int in_width, int in_height)
    {
        width = std::max(1, in_width);
        height = std::max(1, in_height);
    }

    float get_aspect_ratio() const
    {
        return static_cast<float>(width) / static_cast<float>(height);
    }
};

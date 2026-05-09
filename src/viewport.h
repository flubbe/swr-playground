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

class Viewport
{
    /** Local viewport camera. */
    Camera local_camera;

    /** Active camera. If `nullptr`, the local camera is used. */
    std::optional<ObjectId> scene_camera_id;

public:
    Viewport() = default;
    Viewport(const Viewport&) = delete;
    Viewport(Viewport&&) = default;

    Viewport& operator=(const Viewport&) = delete;
    Viewport& operator=(Viewport&&) = default;

    /** Get the viewport camera. Falls back to the local camera if the scene camera cannot be found. */
    Camera& get_camera(Scene& scene)
    {
        if(scene_camera_id.has_value())
        {
            if(Camera* camera = scene.find_camera(scene_camera_id.value()))
            {
                return *camera;
            }
        }

        return local_camera;
    }

    /** Get the viewport-local camera. */
    Camera& get_local_camera()
    {
        return local_camera;
    }

    /** Whether we are using a viewport-local camera. Does not check if a scene camera exists in the scene. */
    bool is_local_camera() const
    {
        return !scene_camera_id.has_value();
    }

    /** Whether we are using a scene camera. Does not check if a scene camera exists in the scene. */
    bool is_scene_camera() const
    {
        return !is_local_camera();
    }

    // TODO clean up.
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

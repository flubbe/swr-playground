#pragma once

#include <algorithm>
#include <optional>

#include "scene/camera.h"
#include "scene/object.h"

/** Viewport display settings (how geometry is rasterized). */
struct ViewportDisplaySettings
{
    /** Whether to render a wireframe view. */
    bool wireframe{false};

    /** Whether to apply face culling. */
    bool cull_face{true};

    /** Whether to skip mesh sections outside the camera frustum. */
    bool cull_frustum{true};
};

/** Viewport overlay settings. */
struct ViewportOverlaySettings
{
    /** Whether to show the camera selector. */
    bool show_camera_selector{true};

    /** Whether to show a grid. */
    bool show_grid{true};
};

/** Viewport render resolution in pixels. */
struct ViewportResolution
{
    /** Viewport width. */
    int width{1};

    /** Viewport height. */
    int height{1};
};

/** Viewport camera type. */
enum class ViewportCameraType : std::uint8_t
{
    Local, /** Viewport controls the camera. */
    Scene  /** Scene update controls the camera. */
};

/** Viewport mouse navigation mode. */
enum class ViewportNavigationMode : std::uint8_t
{
    Fps,   /** Right mouse look with WASD-style movement. */
    Orbit  /** Right mouse orbit around a focus point. */
};

class Viewport
{
    /** Local viewport camera. */
    Camera local_camera;

    /** Active camera. If `nullptr`, the local camera is used. */
    std::optional<ObjectId> scene_camera_id;

    /** Display/rasterization settings. */
    ViewportDisplaySettings display_settings;

    /** Overlay settings. */
    ViewportOverlaySettings overlay_settings;

    /** Mouse navigation mode. */
    ViewportNavigationMode navigation_mode{ViewportNavigationMode::Fps};

    /** Render target resolution. */
    ViewportResolution resolution;

public:
    Viewport() = default;
    Viewport(const Viewport&) = delete;
    Viewport(Viewport&&) = default;

    Viewport& operator=(const Viewport&) = delete;
    Viewport& operator=(Viewport&&) = default;

    /** Use the local viewport camera. */
    void use_local_camera()
    {
        scene_camera_id.reset();
    }

    /**
     * Use a scene-defined camera.
     *
     * @param camera_id Object id of the camera in the scene.
     */
    void use_scene_camera(
      ObjectId camera_id)
    {
        scene_camera_id = camera_id;
    }

    /** Return the object id for the scene camera, or `std::nullopt`. */
    std::optional<ObjectId> get_scene_camera_id() const
    {
        return scene_camera_id;
    }

    /**
     * Try to get the the camera from the scene.
     *
     * @param scene The scene containing the camera.
     * @return The camera, or `nullptr` if either there is no scene camera set, or
     *         the camera is not found in the scene.
     */
    Camera* try_get_scene_camera(Scene& scene)
    {
        if(!scene_camera_id.has_value())
        {
            return nullptr;
        }
        return scene.find_camera(
          scene_camera_id.value());
    }

    /**
     * Try to get the the camera from the scene.
     *
     * @param scene The scene containing the camera.
     * @return The camera, or `nullptr` if either there is no scene camera set, or
     *         the camera is not found in the scene.
     */
    const Camera* try_get_scene_camera(const Scene& scene) const
    {
        if(!scene_camera_id.has_value())
        {
            return nullptr;
        }
        return scene.find_camera(
          scene_camera_id.value());
    }

    /** Get the viewport camera. Falls back to the local camera if the scene camera cannot be found. */
    Camera& get_camera(Scene& scene)
    {
        if(Camera* camera = try_get_scene_camera(scene))
        {
            return *camera;
        }

        return local_camera;
    }

    /** Get the active viewport camera (const). Falls back to local if scene camera cannot be found. */
    const Camera& get_camera(const Scene& scene) const
    {
        if(const Camera* camera = try_get_scene_camera(scene))
        {
            return *camera;
        }

        return local_camera;
    }

    /** Get the viewport-local camera. */
    Camera& get_local_camera()
    {
        return local_camera;
    }

    /** Get the viewport-local camera (const). */
    const Camera& get_local_camera() const
    {
        return local_camera;
    }

    /** Ensure the active camera projection matches current viewport aspect ratio. */
    void update_active_camera_projection(Scene& scene)
    {
        get_camera(scene).update_projection_matrix(get_aspect_ratio());
    }

    /** Return the active camera type for this viewport. */
    ViewportCameraType get_camera_type(const Scene& scene) const
    {
        if(try_get_scene_camera(scene) != nullptr)
        {
            return ViewportCameraType::Scene;
        }
        return ViewportCameraType::Local;
    }

    /** Return the display settings for this viewport. */
    const ViewportDisplaySettings& get_display_settings() const
    {
        return display_settings;
    }

    /** Set the display settings for this viewport. */
    void set_display_settings(ViewportDisplaySettings settings)
    {
        display_settings = settings;
    }

    /** Get the overlay settings for this viewport. */
    const ViewportOverlaySettings& get_overlay_settings() const
    {
        return overlay_settings;
    }

    /** Set the overlay settings for this viewport. */
    void set_overlay_settings(ViewportOverlaySettings settings)
    {
        overlay_settings = settings;
    }

    /** Return the mouse navigation mode. */
    ViewportNavigationMode get_navigation_mode() const
    {
        return navigation_mode;
    }

    /** Set the mouse navigation mode. */
    void set_navigation_mode(
      ViewportNavigationMode mode)
    {
        navigation_mode = mode;
    }

    /** Whether the camera selector overlay is enabled. */
    bool is_camera_selector_overlay_enabled() const
    {
        return overlay_settings.show_camera_selector;
    }

    /** Set whether to show or hide the camera selector overlay. */
    void set_camera_selector_overlay_enabled(bool enabled)
    {
        overlay_settings.show_camera_selector = enabled;
    }

    /** Return the viewport resolution. */
    ViewportResolution get_resolution() const
    {
        return resolution;
    }

    /**
     * Set the viewport resolution.
     *
     * @note Internally, the function takes the maximum of `1` and `width` (resp. `height`).
     *
     * @param width The new width.
     * @param height The new height.
     */
    void set_resolution(int width, int height)
    {
        resolution.width = std::max(1, width);
        resolution.height = std::max(1, height);
    }

    /** Return the aspect ratio of this viewport. */
    float get_aspect_ratio() const
    {
        return static_cast<float>(resolution.width)
               / static_cast<float>(resolution.height);
    }
};

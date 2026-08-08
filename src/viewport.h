/**
 * Software Rasterizer Playground.
 *
 * Viewport.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

#include <ml/all.h>

#include "scene/camera.h"
#include "scene/object.h"
#include "scene/scene.h"

/** Viewport display settings (how geometry is rasterized). */
struct ViewportDisplaySettings
{
    /** Whether to render a wireframe view. */
    bool wireframe{false};

    /** Whether to apply face culling. */
    bool cull_face{true};

    /** Whether to skip mesh sections outside the camera frustum. */
    bool cull_frustum{true};

    /** Whether to select static mesh LODs from projected screen size. */
    bool dynamic_lod{true};

    /** LOD pixels per triangle target. */
    float target_pixels_per_triangle{2.f};

    /** Whether to sort mesh submissions from front to back. */
    bool sort_meshes{true};

    /** Whether to display the spotlight shadow-map depth texture. */
    bool debug_spotlight_depth{false};
};

/** Viewport overlay settings. */
struct ViewportOverlaySettings
{
    /** Whether to show the camera selector. */
    bool show_camera_selector{true};

    /** Whether to show a grid. */
    bool show_grid{false};
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
    Fps,  /** Right mouse look with WASD-style movement. */
    Orbit /** Right mouse orbit around a focus point. */
};

/** Preset editor camera view for the viewport-local camera. */
enum class EditorCameraView : std::uint8_t
{
    Perspective = 0,
    Top,
    Left,
    Front,
    Orthographic,
};

/** Convert `EditorCameraView` to string. */
std::string_view to_string(EditorCameraView view);

/** Input for updating the viewport-local editor camera. */
struct ViewportEditorCameraInput
{
    float move_forward{0.f};
    float move_right{0.f};
    float move_up{0.f};
    float look_yaw{0.f};
    float look_pitch{0.f};
    float zoom_delta{0.f};
    bool fast_move{false};
    bool active{false};
};

class Viewport
{
public:
    struct EditorCameraControllerState
    {
        ml::vec3 position{0.f, 0.f, 40.f};
        ml::vec3 orbit_target{0.f, 0.f, 0.f};
        float orbit_distance{40.f};
        float orthographic_height{40.f};
        float pitch_radians{ml::to_radians(20.f)};
        float yaw_radians{ml::to_radians(30.f)};
    };

private:
    /** Local viewport camera. */
    Camera local_camera;

    /** Selected camera source for this viewport. */
    ViewportCameraType camera_selection{ViewportCameraType::Local};

    /** Selected scene camera id, when the viewport is looking through a scene camera. */
    std::optional<ObjectId> scene_camera_id;

    /** Display/rasterization settings. */
    ViewportDisplaySettings display_settings;

    /** Overlay settings. */
    ViewportOverlaySettings overlay_settings;

    /** Mouse navigation mode. */
    ViewportNavigationMode navigation_mode{ViewportNavigationMode::Orbit};

    /** Current editor-local camera preset. */
    EditorCameraView editor_camera_view{EditorCameraView::Perspective};

    /** Editor controller state for the viewport-local camera. */
    EditorCameraControllerState editor_camera_controller{};

    /** Cached navigation mode for cross-mode synchronization. */
    std::uint8_t last_navigation_mode{
      static_cast<std::uint8_t>(ViewportNavigationMode::Orbit)};

    /** Whether the local editor camera responds to user input. */
    bool editor_camera_modification_enabled{true};

    /** Render target resolution. */
    ViewportResolution resolution;

    void sync_local_camera();
    void apply_editor_camera_view(
      EditorCameraView view,
      bool reset_controller);

public:
    Viewport();
    Viewport(const Viewport&) = delete;
    Viewport(Viewport&&) = default;

    Viewport& operator=(const Viewport&) = delete;
    Viewport& operator=(Viewport&&) = default;

    /** Use the local viewport camera. */
    void use_local_camera()
    {
        camera_selection = ViewportCameraType::Local;
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
        camera_selection = ViewportCameraType::Scene;
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
     * @returns The camera, or `nullptr` if either there is no scene camera set, or
     *         the camera is not found in the scene.
     */
    Camera* try_get_scene_camera(Scene& scene)
    {
        if(camera_selection != ViewportCameraType::Scene
           || !scene_camera_id.has_value())
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
     * @returns The camera, or `nullptr` if either there is no scene camera set, or
     *         the camera is not found in the scene.
     */
    const Camera* try_get_scene_camera(const Scene& scene) const
    {
        if(camera_selection != ViewportCameraType::Scene
           || !scene_camera_id.has_value())
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

    /** Return whether the given editor view is currently the active viewport camera. */
    bool is_editor_camera_view_active(
      const Scene& scene,
      EditorCameraView view) const
    {
        return get_camera_type(scene) == ViewportCameraType::Local
               && editor_camera_view == view;
    }

    /** Return whether the given scene camera is currently active in the viewport. */
    bool is_scene_camera_active(
      const Scene& scene,
      ObjectId camera_id) const
    {
        const Camera* camera = try_get_scene_camera(scene);
        return camera != nullptr && camera->get_object_id() == camera_id;
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

    /** Reset the viewport-local editor camera for the current editor view preset. */
    void reset_editor_camera();

    /** Set the viewport-local editor camera preset. */
    void set_editor_camera_view(EditorCameraView view);

    /** Return the current viewport-local editor camera preset. */
    EditorCameraView get_editor_camera_view() const noexcept
    {
        return editor_camera_view;
    }

    /** Return whether the viewport is actively using its local editor camera. */
    bool is_local_camera_active(const Scene& scene) const;

    /** Return whether editor-camera input is enabled for this viewport. */
    bool is_editor_camera_modification_enabled() const noexcept
    {
        return editor_camera_modification_enabled;
    }

    /** Set whether editor-camera input is enabled for this viewport. */
    void set_editor_camera_modification_enabled(bool enabled)
    {
        editor_camera_modification_enabled = enabled;
    }

    /** Update the viewport-local editor camera from user input. */
    void update_editor_camera(
      float delta_time,
      const ViewportEditorCameraInput& input);

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

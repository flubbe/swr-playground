/**
 * Software Rasterizer Playground.
 *
 * Scene description.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <format>
#include <type_traits>
#include <utility>

#include <ml/all.h>

#include "containers/memory.h"
#include "containers/unordered_map.h"
#include "containers/unordered_set.h"
#include "containers/vector.h"
#include "reflection/cast.h"
#include "reflection/construct.h"
#include "systems/system.h"
#include "animation.h"
#include "camera.h"
#include "directional_light.h"
#include "spotlight.h"
#include "object.h"

/** Scene description. */
class Scene
{
    /** Scene objects. */
    swr::vector<
      reflect::unique_ptr<Object>>
      objects;

    /** Scene update systems. */
    swr::vector<
      swr::unique_ptr<SceneSystem>>
      systems;

    /** Automatic name tracking. */
    swr::unordered_map<
      const reflect::ClassInfo*,
      std::uint32_t>
      object_name_counters;

    /** Object id tracking. */
    std::uint32_t next_id{0};

    /** Per-object spin animations. */
    swr::unordered_map<
      ObjectId,
      SpinAnimation>
      spin_animations;

    /** Map object id into objects list. */
    swr::unordered_map<
      ObjectId,
      Object*>
      objects_by_id;

    /** Scene time. */
    float time{0};

    /** Whether to update. */
    bool paused{false};

    /** Dirty meshes, as object id's. */
    swr::unordered_set<ObjectId> dirty_meshes;

public:
    /** Default constructor. */
    Scene() = default;

    /**
     * Disable copy constructor.
     *
     * @note If the copy should be allowed, we need to take care of e.g. Scene-Object
     *     relations (see e.g. `Object::set_scene`).
     */
    Scene(const Scene&) = delete;

    /** Move constructor. */
    Scene(
      Scene&& other)
    : objects{std::move(other.objects)}
    , systems{std::move(other.systems)}
    , object_name_counters{std::move(other.object_name_counters)}
    , next_id{other.next_id}
    , spin_animations{std::move(other.spin_animations)}
    , objects_by_id{std::move(other.objects_by_id)}
    , time{other.time}
    , paused{other.paused}
    , dirty_meshes{std::move(other.dirty_meshes)}
    {
        for(auto& object: objects)
        {
            object->set_scene(this);
        }
    }

    /**
     * Disable copy assignment.
     *
     * @note If the copy should be allowed, we need to take care of e.g. Scene-Object
     *     relations (see e.g. `Object::set_scene`).
     */
    Scene& operator=(const Scene&) = delete;

    /**
     * Disable move assignment.
     *
     * @note If the move should be allowed, we need to take care of e.g. Scene-Object
     *     relations (see e.g. `Object::set_scene`).
     */
    Scene& operator=(Scene&&) = delete;

    /*
     * Scene-global state.
     */

    /** Set whether the scene is paused */
    void set_paused(
      bool in_pause)
    {
        paused = in_pause;
    }

    /** Return whether the scene is paused. */
    bool is_paused() const
    {
        return paused;
    }

    /** Return the scene time. */
    float get_time() const
    {
        return time;
    }

    /** Set the scene time. */
    void set_time(
      float new_time)
    {
        time = new_time;
    }

    /** Clear the scene. */
    void clear();

    /** Replace the active scene contents with a staged scene. */
    void replace(Scene&& other)
    {
        clear();

        objects = std::move(other.objects);
        systems = std::move(other.systems);
        object_name_counters = std::move(other.object_name_counters);
        next_id = other.next_id;
        spin_animations = std::move(other.spin_animations);
        objects_by_id = std::move(other.objects_by_id);
        time = other.time;
        paused = other.paused;
        dirty_meshes = std::move(other.dirty_meshes);

        for(auto& object: objects)
        {
            object->set_scene(this);
        }
    }

    /**
     * Tick the scene. Updates the scene time.
     *
     * @param delta_time Time passed since the last tick, in seconds.
     */
    void tick(
      float delta_time);

    /*
     * Scene systems.
     */

    /**
     * Set up a spin animation for an object.
     *
     * @param object_id The object to spin.
     * @param animation Spin animation description.
     */
    void set_spin_animation(
      ObjectId object_id,
      SpinAnimation animation);

    /**
     * Remove a spin animation.
     *
     * @param object_id The object to remove the animation from.
     * @note No-op if the object doesn't have an associated animation.
     */
    void remove_spin_animation(
      ObjectId object_id);

    /*
     * Object management.
     */

    /**
     * Find an object by id.
     *
     * @param id The object id.
     * @returns Returns the object, or `nullptr` if not found.
     */
    Object* find_object(
      ObjectId id);

    /**
     * Find an object by id.
     *
     * @param id The object id.
     * @returns Returns the object, or `nullptr` if not found.
     */
    const Object* find_object(
      ObjectId id) const;

    /**
     * Find an object by id and cast the the requested type.
     *
     * @tparam T The object type. Has to be a subclass of `Object`.
     * @param id The object id.
     * @returns Returns the object, or `nullptr` if not found or the type's don't match.
     */
    template<typename T>
        requires std::derived_from<T, Object>
    T* find_object(
      ObjectId id)
    {
        return reflect::try_cast<T>(find_object(id));
    }

    /**
     * Find an object by id and cast the the requested type.
     *
     * @tparam T The object type. Has to be a subclass of `Object`.
     * @param id The object id.
     * @returns Returns the object, or `nullptr` if not found or the type's don't match.
     */
    template<typename T>
        requires std::derived_from<T, Object>
    const T* find_object(
      ObjectId id) const
    {
        return reflect::try_cast<T>(find_object(id));
    }

    /**
     * Find a camera by id.
     *
     * @param id The camera id.
     * @returns Returns the camera, or `nullptr` if not found or if the
     *     referenced object is not a camera.
     */
    Camera* find_camera(
      ObjectId id);

    /**
     * Find a camera by id.
     *
     * @param id The camera id.
     * @returns Returns the camera, or `nullptr` if not found or if the
     *     referenced object is not a camera.
     */
    const Camera* find_camera(
      ObjectId id) const;

    /**
     * Iterates over all stored objects matching or derived from type `T`.
     * Calls the provided callback for each matching object downcasted to `T`.
     *
     * @tparam T Object type to filter by (must derive from `Object`).
     * @param fn Callback function with one of the following signatures:
     *           - `void(T&)` or `void(T&, std::size_t index)`
     *           - `bool(T&)` or `bool(T&, std::size_t index)`
     *
     * @note If the callback returns a type convertible to `bool`, returning `false`
     *       will terminate iteration early. Returning `void` iterates all objects.
     */
    template<typename T, typename Fn>
        requires std::derived_from<T, Object>
    void for_each_object(Fn&& fn)
    {
        constexpr bool takes_index = std::is_invocable_v<Fn&, T&, std::size_t>;

        std::size_t index = 0;
        for(auto& object: objects)
        {
            if(auto* typed = reflect::try_cast<T>(object.get()))
            {
                if constexpr(takes_index)
                {
                    using Ret = std::invoke_result_t<Fn&, T&, std::size_t>;

                    if constexpr(std::is_same_v<Ret, void>)
                    {
                        std::forward<Fn>(fn)(*typed, index++);
                    }
                    else
                    {
                        if(!std::forward<Fn>(fn)(*typed, index++))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    using Ret = std::invoke_result_t<Fn&, T&>;

                    if constexpr(std::is_same_v<Ret, void>)
                    {
                        std::forward<Fn>(fn)(*typed);
                    }
                    else
                    {
                        if(!std::forward<Fn>(fn)(*typed))
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    /**
     * Iterates over all stored objects matching or derived from type `T`.
     * Calls the provided callback for each matching object downcasted to `T`.
     *
     * @tparam T Object type to filter by (must derive from `Object`).
     * @param fn Callback function with one of the following signatures:
     *           - `void(const T&)` or `void(const T&, std::size_t index)`
     *           - `bool(const T&)` or `bool(const T&, std::size_t index)`
     *
     * @note If the callback returns a type convertible to `bool`, returning `false`
     *       will terminate iteration early. Returning `void` iterates all objects.
     */
    template<typename T, typename Fn>
        requires std::derived_from<T, Object>
    void for_each_object(Fn&& fn) const
    {
        constexpr bool takes_index = std::is_invocable_v<Fn&, const T&, std::size_t>;

        std::size_t index = 0;
        for(const auto& object: objects)
        {
            if(const auto* typed = reflect::try_cast<const T>(object.get()))
            {
                if constexpr(takes_index)
                {
                    using Ret = std::invoke_result_t<Fn&, const T&, std::size_t>;

                    if constexpr(std::is_same_v<Ret, void>)
                    {
                        std::forward<Fn>(fn)(*typed, index++);
                    }
                    else
                    {
                        if(!std::forward<Fn>(fn)(*typed, index++))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    using Ret = std::invoke_result_t<Fn&, const T&>;

                    if constexpr(std::is_same_v<Ret, void>)
                    {
                        std::forward<Fn>(fn)(*typed);
                    }
                    else
                    {
                        if(!std::forward<Fn>(fn)(*typed))
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    /** Returns a filtered, downcasted view over objects derived from `T`. */
    template<typename T>
        requires std::derived_from<T, Object>
    auto objects_of()
    {
        return objects
               | std::views::transform([](auto& ptr)
                                       { return reflect::try_cast<T>(ptr.get()); })
               | std::views::filter([](T* ptr)
                                    { return ptr != nullptr; })
               | std::views::transform([](T* ptr) -> T&
                                       { return *ptr; });
    }

    /** Const overload returning a view of `const T&`. */
    template<typename T>
        requires std::derived_from<T, Object>
    auto objects_of() const
    {
        return objects
               | std::views::transform([](const auto& ptr)
                                       { return reflect::try_cast<T>(ptr.get()); })
               | std::views::filter([](const T* ptr)
                                    { return ptr != nullptr; })
               | std::views::transform([](const T* ptr) -> const T&
                                       { return *ptr; });
    }

    /**
     * Create a new object.
     *
     * @tparam T The object type. Must be a subclass of `Object`.
     * @tparam Args Initialization parameters. Forwarded to `T::init` if the method exists.
     *     Must be empty if `T::init` does not exist.
     * @return Returns a pointer to the created object.
     */
    template<typename T, typename... Args>
        requires(
          std::is_base_of_v<Object, T>)
    T* create_object(Args&&... args)
    {
        return create_object<T>(
          reflect::construct_and_init<Object, T>(
            std::forward<Args>(args)...));
    }

    /**
     * Create a scene object from `obj`.
     *
     * @tparam T The object type. Must be a subclass of `Object`.
     * @param obj The object's data, which is moved into the scene's object list.
     * @return Returns a pointer to the object.
     *
     * @note Overwrites `obj`'s id and name.
     */
    template<typename T>
        requires(
          std::is_base_of_v<Object, T>)
    T* create_object(reflect::unique_ptr<T> obj)
    {
        T* ptr = obj.get();

        // set object id and name,
        std::uint32_t object_id = ++next_id;

        const auto* class_info = T::static_class();
        std::uint32_t name_counter = ++object_name_counters[class_info];

        ptr->set_object_id(make_object_id(object_id));
        ptr->set_name(
          std::format(
            "{}_{}",
            class_info->name,
            name_counter));
        ptr->capture_snapshot();

        add_object(std::move(obj));

        return ptr;
    }

    /**
     * Add an object to the scene.
     *
     * @param obj The object to add.
     * @throws Throws an `std::runtime_error` if an object with the same id
     *     already exists.
     */
    void add_object(
      reflect::unique_ptr<Object> obj)
    {
        auto object_ptr = obj.get();
        auto object_id = obj->get_object_id();

        if(objects_by_id.contains(object_id))
        {
            throw std::runtime_error{
              std::format(
                "Object with id '{}' already exists in scene.",
                object_id.value)};
        }

        objects.emplace_back(std::move(obj));
        objects_by_id.emplace(object_id, object_ptr);

        object_ptr->set_scene(this);
    }

    /**
     * Add a new system to the scene.
     *
     * @tparam T The system type. Must be a subclass of `SceneSystem`.
     * @tparam Args Forwarded constructor parameters for `T{...}`.
     * @returns Returns a pointer to the created system.
     */
    template<typename T, typename... Args>
        requires(
          std::is_base_of_v<SceneSystem, T>)
    T* add_system(Args&&... args)
    {
        auto system = swr::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        systems.emplace_back(std::move(system));
        return ptr;
    }

    /*
     * Mesh management.
     */

    /** Clear dirty mesh list. */
    void clear_dirty_meshes()
    {
        dirty_meshes.clear();
    }

    /** Return the dirty meshes, via object id. */
    const swr::unordered_set<ObjectId>& get_dirty_meshes() const
    {
        return dirty_meshes;
    }

    /**
     * Mark a mesh as dirty.
     *
     * @param object_id The mesh object id.
     */
    void mark_mesh_dirty(
      ObjectId object_id)
    {
        dirty_meshes.insert(object_id);
    }

    /*
     * Import and export.
     */

    /**
     * Save the scene to JSON.
     *
     * @param indentation_size Indentation size. Defaults to 4.
     * @param use_compacted_format Whether to use a compacted format: no indentation, no newlines.
     *     Defaults to `false`.
     * @returns Returns the JSON description of the scene.
     * @throws `std::runtime_error` if saving fails.
     */
    swr::string save(
      std::size_t indentation_size = 4,
      bool use_compacted_format = false) const;

    /*
     * Accessors.
     */

    /** Return all scene objects. */
    swr::vector<reflect::unique_ptr<Object>>& get_objects()
    {
        return objects;
    }

    /** Return all scene objects. */
    const swr::vector<reflect::unique_ptr<Object>>& get_objects() const
    {
        return objects;
    }

    /** Return a map of object id's to object pointers. */
    const swr::unordered_map<ObjectId, Object*>& get_objects_by_id() const
    {
        return objects_by_id;
    }

    /** Return the spin animations. */
    const swr::unordered_map<ObjectId, SpinAnimation>& get_spin_animations() const
    {
        return spin_animations;
    }
};

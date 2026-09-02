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

class Scene
{
    /** scene objects. */
    swr::vector<reflect::unique_ptr<Object>> objects;

    /** scene update systems. */
    swr::vector<swr::unique_ptr<SceneSystem>> systems;

    /** automatic name tracking. */
    swr::unordered_map<
      const reflect::ClassInfo*,
      std::uint32_t>
      object_name_counters;

    /** object id tracking. */
    std::uint32_t next_id{0};

    /** per-object spin animations. */
    swr::unordered_map<
      ObjectId,
      SpinAnimation>
      spin_animations;

    /** map object id into objects list. */
    swr::unordered_map<
      ObjectId,
      Object*>
      objects_by_id;

    /** scene time. */
    float time{0};

    /** whether to update. */
    bool paused{false};

    /** Dirty meshes, as object id's. */
    swr::unordered_set<ObjectId> dirty_meshes;

public:
    Scene();

    void set_paused(bool in_pause)
    {
        paused = in_pause;
    }

    bool is_paused() const
    {
        return paused;
    }

    float get_time() const
    {
        return time;
    }

    void set_time(float new_time)
    {
        time = new_time;
    }

    void clear();
    void tick(float delta_time);

    void add_default_systems();

    void set_spin_animation(
      ObjectId object_id,
      SpinAnimation animation);
    void remove_spin_animation(ObjectId object_id);

    Object* find_object(ObjectId id);
    const Object* find_object(ObjectId id) const;

    template<typename T>
        requires std::derived_from<T, Object>
    T* find_object(ObjectId id)
    {
        return reflect::try_cast<T>(find_object(id));
    }

    template<typename T>
        requires std::derived_from<T, Object>
    const T* find_object(ObjectId id) const
    {
        return reflect::try_cast<T>(find_object(id));
    }

    Camera* find_camera(ObjectId id);
    const Camera* find_camera(ObjectId id) const;

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

    template<typename T, typename... Args>
        requires(
          std::is_base_of_v<Object, T>)
    T* create_object(Args&&... args)
    {
        return create_object<T>(
          reflect::construct_and_init<Object, T>(
            std::forward<Args>(args)...));
    }

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
    void mark_mesh_dirty(ObjectId object_id)
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

    const swr::vector<reflect::unique_ptr<Object>>& get_objects() const
    {
        return objects;
    }

    swr::vector<reflect::unique_ptr<Object>>& get_objects()
    {
        return objects;
    }

    const swr::unordered_map<ObjectId, SpinAnimation>& get_spin_animations() const
    {
        return spin_animations;
    }

    const swr::unordered_map<ObjectId, Object*>& get_objects_by_id() const
    {
        return objects_by_id;
    }
};

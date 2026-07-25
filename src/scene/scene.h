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
#include <memory>
#include <type_traits>
#include <utility>

#include "ml/all.h"

#include "containers/unordered_map.h"
#include "containers/vector.h"
#include "reflection/cast.h"
#include "systems/system.h"
#include "animation.h"
#include "camera.h"
#include "directionallight.h"
#include "spotlight.h"
#include "object.h"

class Scene
{
    /** scene objects. */
    swr::vector<std::unique_ptr<Object>> objects;

    /** scene update systems. */
    swr::vector<std::unique_ptr<SceneSystem>> systems;

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
        requires(
          std::is_base_of_v<Object, T>)
    T* find_object(ObjectId id)
    {
        return reflect::try_cast<T>(find_object(id));
    }

    template<typename T>
        requires(
          std::is_base_of_v<Object, T>)
    const T* find_object(ObjectId id) const
    {
        return reflect::try_cast<T>(find_object(id));
    }

    Camera* find_camera(ObjectId id);
    const Camera* find_camera(ObjectId id) const;
    void get_cameras(swr::vector<Camera*>& cameras);
    void get_cameras(swr::vector<const Camera*>& cameras) const;
    void get_directional_lights(swr::vector<DirectionalLight*>& lights);
    void get_directional_lights(swr::vector<const DirectionalLight*>& lights) const;
    void get_spot_lights(swr::vector<SpotLight*>& spot_lights);
    void get_spot_lights(swr::vector<const SpotLight*>& spot_lights) const;

    template<typename T, typename Fn>
        requires(
          std::is_base_of_v<Object, T>)
    void for_each_object(Fn&& fn)
    {
        for(auto& object: objects)
        {
            if(auto* typed = reflect::try_cast<T>(object.get()))
            {
                fn(*typed);
            }
        }
    }

    template<typename T, typename Fn>
        requires(
          std::is_base_of_v<Object, T>)
    void for_each_object(Fn&& fn) const
    {
        for(const auto& object: objects)
        {
            if(const auto* typed = reflect::try_cast<T>(object.get()))
            {
                fn(*typed);
            }
        }
    }

    template<typename T, typename... Args>
        requires(
          std::is_base_of_v<Object, T>)
    T* add_object(Args&&... args)
    {
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();
        objects.emplace_back(std::move(obj));

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
        objects_by_id.emplace(ptr->get_object_id(), ptr);

        return ptr;
    }

    template<typename T, typename... Args>
        requires(
          std::is_base_of_v<SceneSystem, T>)
    T* add_system(Args&&... args)
    {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        systems.emplace_back(std::move(system));
        return ptr;
    }

    const swr::vector<std::unique_ptr<Object>>& get_objects() const
    {
        return objects;
    }

    swr::vector<std::unique_ptr<Object>>& get_objects()
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

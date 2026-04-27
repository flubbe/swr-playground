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

#include <memory>
#include <type_traits>
#include <vector>

#include "ml/all.h"

#include "camera.h"
#include "light.h"
#include "object.h"

class Scene
{
    /** scene objects. */
    std::vector<std::unique_ptr<Object>> objects;

    /** object id tracking. */
    std::unordered_map<const reflect::ClassInfo*, uint32_t> next_ids;

    /** scene light. */
    Light light;

    /** scene time. */
    float time{0};

    /** whether to update. */
    bool paused{false};

public:
    Scene() = default;

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

    template<typename T, typename... Args>
        requires(
          std::is_base_of_v<Object, T>)
    T* add_object(Args&&... args)
    {
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();
        objects.emplace_back(std::move(obj));

        // set object id and name,
        const auto* class_info = T::static_class();
        uint32_t object_id = ++next_ids[class_info];

        ptr->set_object_id(make_object_id(object_id));
        ptr->set_name(
          std::format(
            "{}_{}",
            class_info->name,
            object_id));

        return ptr;
    }

    const std::vector<std::unique_ptr<Object>>& get_objects() const
    {
        return objects;
    }

    std::vector<std::unique_ptr<Object>>& get_objects()
    {
        return objects;
    }

    const Light& get_light() const
    {
        return light;
    }
};

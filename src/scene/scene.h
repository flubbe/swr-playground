/**
 * Software Rasterizer Playground.
 *
 * A scene description.
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

    /** scene light. */
    Light light;

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

    template<typename T, typename... Args>
        requires(
          std::is_base_of_v<Object, T>)
    T* add_object(Args&&... args)
    {
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();
        objects.emplace_back(std::move(obj));

        return ptr;
    }

    const std::vector<std::unique_ptr<Object>>& get_objects() const
    {
        return objects;
    }

    const Light& get_light() const
    {
        return light;
    }
};
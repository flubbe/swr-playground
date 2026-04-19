#pragma once

#include "swr/shaders.h"

class ShaderCache
{
    std::vector<std::unique_ptr<swr::program_base>> shaders;

public:
    ShaderCache() = default;

    template<typename T, typename... Args>
    T* add(Args&&... args)
    {
        std::unique_ptr<T> new_shader = std::make_unique<T>(std::forward<Args>(args)...);
        T* shader = new_shader.get();

        shaders.emplace_back(std::move(new_shader));
        return shader;
    }

    void clear()
    {
        shaders.clear();
    }
};

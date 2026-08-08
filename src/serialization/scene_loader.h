#pragma once

#include <string_view>

class Scene;

namespace serial
{

class SceneLoader
{
public:
    virtual ~SceneLoader() = default;
    virtual void load(
      Scene& scene,
      std::string_view source_text) = 0;
};

}    // namespace serial

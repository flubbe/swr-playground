#include <simdjson.h>

#include "json_scene_loader.h"

namespace serial::json
{

void JsonSceneLoader::load(
  Scene& scene,
  std::string_view source_text)
{
    scene.clear();

    simdjson::padded_string padded_json(source_text);
    simdjson::ondemand::parser json_parser;

    simdjson::ondemand::document doc;
    if(auto err = json_parser.iterate(padded_json).get(doc); err)
    {
        throw std::runtime_error{
          std::format("JSON parsing failed: {}", simdjson::error_message(err))};
    }

    simdjson::ondemand::object root_obj;
    if(auto err = doc.get_object().get(root_obj); err)
    {
        throw std::runtime_error{"Root JSON value must be an object."};
    }

    for(auto field: root_obj)
    {
        std::string_view key = field.unescaped_key();

        if(key == "time")
        {
            scene.set_time(field.value().get_double());
        }
        else if(key == "paused")
        {
            scene.set_paused(field.value().get_bool());
        }
        else if(key == "objects")
        {
            load_objects(scene, field.value().get_array());
        }
        else
        {
            logging::warningf("Unknown key '{}' found in scene", key);
        }
    }
}

}    // namespace serial::json

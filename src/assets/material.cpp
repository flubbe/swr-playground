/**
 * Software Rasterizer Playground.
 *
 * Material asset loading helpers.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <format>

#include <simdjson.h>

#include "logging.h"
#include "material.h"

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"Assets"};
    return logger;
}

namespace assets
{

MaterialDesc load_material(
  std::string_view json)
{
    MaterialDesc desc;

    simdjson::padded_string padded_json{json};
    simdjson::ondemand::parser json_parser;

    simdjson::ondemand::document doc;
    auto error = json_parser.iterate(padded_json).get(doc);
    if(error)
    {
        throw std::runtime_error{
          std::format(
            "JSON parsing failed: {}",
            simdjson::error_message(error))};
    }

    simdjson::ondemand::object root_obj;
    if(doc.get_object().get(root_obj))
    {
        throw std::runtime_error{
          "Root JSON value must be an object."};
    }

    for(auto field: root_obj)
    {
        std::string_view key = field.unescaped_key();

        if(key == "shader")
        {
            desc.shader = swr::string_from(field.value().get_string());
        }
        else if(key == "textures")
        {
            simdjson::ondemand::array arr = field.value().get_array();

            for(auto item: arr)
            {
                desc.textures.emplace_back(
                  swr::string_from(item.value().get_string()));
            }
        }
        else
        {
            get_logger().warningf(
              "Unknown key '{}' found in material.",
              key);
        }
    }

    return desc;
}

}    // namespace assets

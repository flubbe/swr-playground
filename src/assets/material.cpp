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
#include <optional>

#include <simdjson.h>

#include "logging.h"
#include "material.h"

namespace
{

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"Assets"};
    return logger;
}

}    // namespace

namespace assets
{

namespace
{

/**
 * Parse the normal map convention string.
 *
 * @param value Normal map convention as a string.
 * @returns Returns a `NormalMapConvention`.
 * @throws Throws a `std::runtime_error` if `value` is neither `opengl` nor `directx`.
 */
NormalMapConvention parse_normal_map_convention(
  std::string_view value)
{
    if(value == "opengl")
    {
        return NormalMapConvention::OpenGL;
    }
    if(value == "directx")
    {
        return NormalMapConvention::DirectX;
    }
    throw std::runtime_error{
      std::format(
        "Unknown normal-map convention '{}'.",
        value)};
}

}    // namespace

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

        if(key == "name")
        {
            desc.name = swr::string_from(field.value().get_string());
        }
        else if(key == "shader")
        {
            desc.shader = swr::string_from(field.value().get_string());
        }
        else if(key == "textures")
        {
            simdjson::ondemand::object textures = field.value().get_object();
            for(auto texture_field: textures)
            {
                const std::string_view texture_key = texture_field.unescaped_key();
                simdjson::ondemand::object texture = texture_field.value().get_object();

                if(texture_key == "base_color")
                {
                    std::optional<std::filesystem::path> path;
                    for(auto property: texture)
                    {
                        const std::string_view property_key = property.unescaped_key();

                        if(property_key == "path")
                        {
                            path = std::filesystem::path{
                              swr::string_from(property.value().get_string())};
                        }
                        else if(property_key == "color_space")
                        {
                            const std::string_view color_space =
                              property.value().get_string();
                            if(color_space != "srgb")
                            {
                                throw std::runtime_error{
                                  std::format(
                                    "Unsupported base-color color space '{}'.",
                                    color_space)};
                            }
                        }
                        else
                        {
                            get_logger().warningf(
                              "Unknown base-color texture key '{}'.",
                              property_key);
                        }
                    }
                    if(!path.has_value())
                    {
                        throw std::runtime_error{
                          "Base-color texture requires a path."};
                    }
                    desc.base_color = std::move(path);
                }
                else if(texture_key == "normal")
                {
                    std::optional<std::filesystem::path> path;
                    NormalMapConvention convention = NormalMapConvention::OpenGL;
                    float scale = 1.f;

                    for(auto property: texture)
                    {
                        const std::string_view property_key = property.unescaped_key();

                        if(property_key == "path")
                        {
                            path = std::filesystem::path{
                              swr::string_from(property.value().get_string())};
                        }
                        else if(property_key == "convention")
                        {
                            convention = parse_normal_map_convention(
                              property.value().get_string());
                        }
                        else if(property_key == "scale")
                        {
                            scale = static_cast<float>(property.value().get_double());
                        }
                        else
                        {
                            get_logger().warningf(
                              "Unknown normal-map texture key '{}'.",
                              property_key);
                        }
                    }
                    if(!path.has_value())
                    {
                        throw std::runtime_error{
                          "Normal-map texture requires a path."};
                    }
                    desc.normal = NormalMapDesc{
                      .path = std::move(*path),
                      .convention = convention,
                      .scale = scale,
                    };
                }
                else
                {
                    get_logger().warningf(
                      "Unknown texture slot '{}' found in material.",
                      texture_key);
                }
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

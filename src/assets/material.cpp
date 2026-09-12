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
 * Parse a `path` entry.
 *
 * @param obj The JSON DOM element.
 * @returns Returns a path if the entry was present.
 */
std::optional<std::filesystem::path> parse_path(
  simdjson::dom::element obj)
{
    auto path = obj["path"];
    if(path.error() == simdjson::NO_SUCH_FIELD)
    {
        return std::nullopt;
    }

    if(auto error = path.error())
    {
        throw std::runtime_error{
          std::format(
            "Failed to read path: {}",
            simdjson::error_message(error))};
    }

    auto value = path.value().get_string();
    if(auto error = value.error())
    {
        throw std::runtime_error{
          std::format(
            "Path must be a string: {}",
            simdjson::error_message(error))};
    }

    return std::filesystem::path{
      swr::string_from(value.value())};
}

/**
 * Parse the color space.
 *
 * @note Only validates the entry.
 * @param obj The JSON DOM element.
 */
void parse_color_space(
  simdjson::dom::element obj)
{
    auto color_space = obj["color_space"];
    if(color_space.error() == simdjson::NO_SUCH_FIELD)
    {
        return;
    }

    if(auto error = color_space.error())
    {
        throw std::runtime_error{
          std::format(
            "Failed to read base-color color space: {}",
            simdjson::error_message(error))};
    }

    auto value = color_space.value().get_string();
    if(auto error = value.error())
    {
        throw std::runtime_error{
          std::format(
            "Color space must be a string: {}",
            simdjson::error_message(error))};
    }

    if(value.value() != "srgb")
    {
        throw std::runtime_error{
          std::format(
            "Unsupported base-color color space '{}'.",
            value.value())};
    }
}

/**
 * Parse a normal map `convention` entry.
 *
 * @param obj The JSON DOM element.
 * @returns Returns normal map convention.
 *     Defaults to `OpenGL` if the entry was not present,
 *     or didn't match `opengl` or `directx`.
 */
NormalMapConvention parse_normal_map_convention(
  simdjson::dom::element obj)
{
    auto convention = obj["convention"];

    if(convention.error() == simdjson::NO_SUCH_FIELD)
    {
        return NormalMapConvention::OpenGL;
    }

    if(auto error = convention.error())
    {
        throw std::runtime_error{
          std::format(
            "Failed to read normal-map convention: {}",
            simdjson::error_message(error))};
    }

    auto value = convention.value().get_string();
    if(auto error = value.error())
    {
        throw std::runtime_error{
          std::format(
            "Normal-map convention must be a string: {}",
            simdjson::error_message(error))};
    }

    if(value.value() == "opengl")
    {
        return NormalMapConvention::OpenGL;
    }

    if(value.value() == "directx")
    {
        return NormalMapConvention::DirectX;
    }

    throw std::runtime_error{
      std::format(
        "Unknown normal-map convention '{}'.",
        value.value())};
}
/**
 * Parse a `scale` entry.
 *
 * @param obj The JSON DOM element.
 * @returns Returns a scale if the entry was present. Defaults to `1.0f`.
 */
float parse_scale(simdjson::dom::element obj)
{
    auto scale = obj["scale"];

    if(scale.error() == simdjson::NO_SUCH_FIELD)
    {
        return 1.f;
    }

    if(auto error = scale.error())
    {
        throw std::runtime_error{
          std::format(
            "Failed to read normal-map scale: {}",
            simdjson::error_message(error))};
    }

    auto value = scale.value().get_double();
    if(auto error = value.error())
    {
        throw std::runtime_error{
          std::format(
            "Normal-map scale must be a number: {}",
            simdjson::error_message(error))};
    }

    return static_cast<float>(value.value());
}

}    // namespace

MaterialDesc load_material(
  std::string_view json)
{
    MaterialDesc desc;

    simdjson::dom::parser json_parser;

    simdjson::dom::element doc;
    if(auto error = json_parser.parse(json).get(doc))
    {
        throw std::runtime_error{
          std::format(
            "JSON parsing failed: {}",
            simdjson::error_message(error))};
    }

    simdjson::dom::object root_obj;
    if(auto error = doc.get_object().get(root_obj); error)
    {
        throw std::runtime_error{
          std::format(
            "Root JSON value must be an object: {}",
            simdjson::error_message(error))};
    }

    if(auto value = root_obj["name"];
       value.error() != simdjson::NO_SUCH_FIELD)
    {
        if(auto error = value.error())
        {
            throw std::runtime_error{
              std::format(
                "Failed to read material name: {}",
                simdjson::error_message(error))};
        }

        auto str_value = value.value().get_string();
        if(auto error = str_value.error())
        {
            throw std::runtime_error{
              std::format(
                "Material name must be a string: {}",
                simdjson::error_message(error))};
        }

        desc.name = swr::string_from(str_value.value());
    }

    if(auto value = root_obj["shader"];
       value.error() != simdjson::NO_SUCH_FIELD)
    {
        if(auto error = value.error())
        {
            throw std::runtime_error{
              std::format(
                "Failed to read material shader: {}",
                simdjson::error_message(error))};
        }

        auto str_value = value.value().get_string();
        if(auto error = str_value.error())
        {
            throw std::runtime_error{
              std::format(
                "Shader name must be a string: {}",
                simdjson::error_message(error))};
        }

        desc.shader = swr::string_from(str_value.value());
    }

    auto textures_value = root_obj["textures"];
    if(textures_value.error() != simdjson::NO_SUCH_FIELD)
    {
        if(auto error = textures_value.error())
        {
            throw std::runtime_error{
              std::format(
                "Failed to read material textures: {}",
                simdjson::error_message(error))};
        }

        simdjson::dom::object textures;
        if(auto error = textures_value.get_object().get(textures))
        {
            throw std::runtime_error{
              std::format(
                "Material 'textures' must be an object: {}",
                simdjson::error_message(error))};
        }

        for(auto texture_field: textures)
        {
            const std::string_view texture_key = texture_field.key;

            simdjson::dom::object texture;
            if(auto error = texture_field.value.get_object().get(texture); error)
            {
                throw std::runtime_error{
                  std::format(
                    "Material texture '{}' must be an object: {}",
                    texture_key,
                    simdjson::error_message(error))};
            }

            if(texture_key == "base_color")
            {
                auto path = parse_path(texture);
                parse_color_space(texture);    // TODO just validates, otherwise unused.

                if(!path.has_value())
                {
                    throw std::runtime_error{
                      "Base-color texture requires a path."};
                }

                for(auto property: texture)
                {
                    if(property.key != "path"
                       && property.key != "color_space")
                    {
                        get_logger().warningf(
                          "Unknown base-color texture key '{}'.",
                          property.key);
                    }
                }

                desc.base_color = std::move(path);
            }
            else if(texture_key == "normal")
            {
                auto path = parse_path(texture);
                auto convention = parse_normal_map_convention(texture);
                float scale = parse_scale(texture);

                if(!path.has_value())
                {
                    throw std::runtime_error{
                      "Normal-map texture requires a path."};
                }

                for(auto property: texture)
                {
                    if(property.key != "path"
                       && property.key != "convention"
                       && property.key != "scale")
                    {
                        get_logger().warningf(
                          "Unknown normal-map key '{}'.",
                          property.key);
                    }
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

    // Detect unknown top-level fields.
    for(auto field: root_obj)
    {
        if(field.key != "name"
           && field.key != "shader"
           && field.key != "textures")
        {
            get_logger().warningf(
              "Unknown key '{}' found in material.",
              field.key);
        }
    }

    return desc;
}

}    // namespace assets

/**
 * Software Rasterizer Playground.
 *
 * Object property deserializer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "reflection/builtin_properties.h"
#include "scene/properties.h"
#include "serialization/except.h"
#include "property_deserializer.h"
#include "logging.h"

namespace serial::json
{

void JsonPropertyDeserializer::visit(
  reflect::Property& property)
{
    if(auto* p = property.try_as<reflect::IntProperty>())
    {
        std::int64_t val{0};
        if(auto err = value.get_int64().get(val); !err)
        {
            p->set_value(static_cast<reflect::IntProperty::Type>(val));
        }
        else
        {
            logger.warningf(
              "'{}': Unable to deserialize int64 '{}.{}' from JSON: {}",
              object.get_name(),
              object.get_class()->name,
              p->get_name(),
              simdjson::error_message(err));
        }
    }
    else if(auto* p = property.try_as<reflect::UIntProperty>())
    {
        std::uint64_t val{0};
        if(auto err = value.get_uint64().get(val); !err)
        {
            p->set_value(static_cast<reflect::UIntProperty::Type>(val));
        }
        else
        {
            logger.warningf(
              "'{}': Unable to deserialize uint64 '{}.{}' from JSON: {}",
              object.get_name(),
              object.get_class()->name,
              p->get_name(),
              simdjson::error_message(err));
        }
    }
    else if(auto* p = property.try_as<reflect::FloatProperty>())
    {
        double val{0.0};
        if(auto err = value.get_double().get(val); !err)
        {
            p->set_value(static_cast<reflect::FloatProperty::Type>(val));
        }
        else
        {
            logger.warningf(
              "'{}': Unable to deserialize float '{}.{}' from JSON: {}",
              object.get_name(),
              object.get_class()->name,
              p->get_name(),
              simdjson::error_message(err));
        }
    }
    else if(auto* p = property.try_as<reflect::BoolProperty>())
    {
        bool val{false};
        if(auto err = value.get_bool().get(val); !err)
        {
            p->set_value(val);
        }
        else
        {
            logger.warningf(
              "'{}': Unable to deserialize bool '{}.{}' from JSON: {}",
              object.get_name(),
              object.get_class()->name,
              p->get_name(),
              simdjson::error_message(err));
        }
    }
    else if(auto* p = property.try_as<reflect::StringProperty>())
    {
        std::string val;
        if(auto err = value.get_string().get(val); !err)
        {
            p->set_value(val);
        }
        else
        {
            logger.warningf(
              "'{}': Unable to deserialize string '{}.{}' from JSON: {}",
              object.get_name(),
              object.get_class()->name,
              p->get_name(),
              simdjson::error_message(err));
        }
    }
#if SWR_CUSTOM_STRING_TYPE
    else if(auto* p = property.try_as<reflect::SwrStringProperty>())
    {
        std::string val;
        if(auto err = value.get_string().get(val); !err)
        {
            p->set_value(val);
        }
        else
        {
            logger.warningf(
              "'{}': Unable to deserialize string '{}.{}' from JSON: {}",
              object.get_name(),
              object.get_class()->name,
              p->get_name(),
              simdjson::error_message(err));
        }
    }
#endif /* SWR_CUSTOM_STRING_TYPE */
    else if(auto* p = property.try_as<reflect::Mat4Property>())
    {
        simdjson::dom::array outer_arr;
        if(auto err = value.get_array().get(outer_arr); !err)
        {
            ml::mat4x4 m;
            std::size_t row_idx = 0;

            for(auto row_elem: outer_arr)
            {
                if(row_idx >= 4)
                {
                    logger.warningf(
                      "Too many rows when deserializing mat4x4 '{}.{}' from JSON: {}",
                      object.get_name(),
                      object.get_class()->name,
                      p->get_name(),
                      simdjson::error_message(err));

                    break;
                }

                simdjson::dom::array inner_arr;
                if(auto inner_err = row_elem.get_array().get(inner_arr); !inner_err)
                {
                    std::size_t col_idx = 0;
                    for(auto col_elem: inner_arr)
                    {
                        double d{};
                        if(!col_elem.get_double().get(d))
                        {
                            if(col_idx < 4)
                            {
                                m.rows[row_idx][col_idx] = static_cast<float>(d);
                            }
                            else
                            {
                                logger.warningf(
                                  "Too many columns when deserializing mat4x4 '{}.{}' from JSON: {}",
                                  object.get_name(),
                                  object.get_class()->name,
                                  p->get_name(),
                                  simdjson::error_message(err));

                                break;
                            }
                        }
                        ++col_idx;
                    }

                    if(col_idx != 4)
                    {
                        logger.warningf(
                          "Too few columns when deserializing mat4x4 '{}.{}' from JSON: {}",
                          object.get_name(),
                          object.get_class()->name,
                          p->get_name(),
                          simdjson::error_message(err));
                    }
                }
                ++row_idx;
            }

            if(row_idx != 4)
            {
                logger.warningf(
                  "Too few rows when deserializing mat4x4 '{}.{}' from JSON: {}",
                  object.get_name(),
                  object.get_class()->name,
                  p->get_name(),
                  simdjson::error_message(err));
            }

            p->set_value(m);
        }
        else
        {
            logger.warningf(
              "'{}': Unable to deserialize mat4 '{}.{}' from JSON: {}",
              object.get_name(),
              object.get_class()->name,
              p->get_name(),
              simdjson::error_message(err));
        }
    }
    else if(auto* p = property.try_as<reflect::Vec4Property>())
    {
        // Deserialize JSON array [x, y, z, w]
        simdjson::dom::array arr;
        if(auto err = value.get_array().get(arr); !err)
        {
            ml::vec4 v{};
            std::size_t idx = 0;
            for(auto elem: arr)
            {
                double d{};
                if(!elem.get_double().get(d))
                {
                    if(idx < 4)
                    {
                        v[idx] = static_cast<float>(d);
                    }
                }
                ++idx;
            }

            if(idx != 4)
            {
                logger.warningf(
                  "'{}': Too few values when deserializing vec4 '{}.{}' from JSON: {}",
                  object.get_name(),
                  object.get_class()->name,
                  p->get_name(),
                  simdjson::error_message(err));
            }

            p->set_value(v);
        }
        else
        {
            logger.warningf(
              "'{}': Unable to deserialize vec4 '{}.{}' from JSON: {}",
              object.get_name(),
              object.get_class()->name,
              p->get_name(),
              simdjson::error_message(err));
        }
    }
    else
    {
        throw serial::SerializationError{
          std::format(
            "Unsupported property type for property '{}'.",
            property.get_name())};
    }
}

}    // namespace serial::json

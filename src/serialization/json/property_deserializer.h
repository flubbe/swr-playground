/**
 * Software Rasterizer Playground.
 *
 * Object property deserializer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <simdjson.h>

#include "reflection/property.h"
#include "scene/object.h"

/*
 * Forward delcarations.
 */
namespace logging
{
class Logger;
}    // namespace logging

namespace serial::json
{

/** Load a value into a property. */
class JsonPropertyDeserializer final
: public reflect::PropertyVisitor
{
    /** Logger. */
    const logging::Logger& logger;

    /** The object. */
    Object& object;

    /** Value to load. */
    simdjson::dom::element value;

public:
    /**
     * Constructor.
     *
     * @param logger The logger for warnings.
     * @param object The object to load the value into.
     * @param value The JSON value to load.
     */
    explicit JsonPropertyDeserializer(
      const logging::Logger& logger,
      Object& object,
      simdjson::dom::element value)
    : logger{logger}
    , object{object}
    , value{value}
    {
    }

    void visit(
      const reflect::Property& property,
      void* storage) override;
};

}    // namespace serial::json

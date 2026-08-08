
/**
 * Software Rasterizer Playground.
 *
 * JSON property writer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "reflection/property.h"
#include "serialization/json/json_writer.h"

namespace serial::json
{

class JsonPropertyWriter final
: public reflect::PropertyVisitor
{
    JsonWriter& writer;

public:
    explicit JsonPropertyWriter(
      JsonWriter& writer);

    void visit(reflect::Property& property) override;
};

}    // namespace serial::json

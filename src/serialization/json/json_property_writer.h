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

/** Write properties to JSON. */
class JsonPropertyWriter final
: public reflect::PropertyVisitor
{
    /** JSON writer. */
    JsonWriter& writer;

public:
    /**
     * Constructor.
     *
     * @param writer The JSON writer to use.
     */
    explicit JsonPropertyWriter(
      JsonWriter& writer);

    void visit(reflect::Property& property) override;
};

}    // namespace serial::json

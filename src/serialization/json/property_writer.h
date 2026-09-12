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
#include "serialization/json/writer.h"

namespace serial::json
{

/** Write properties to JSON. */
class JsonPropertyWriter final
: public reflect::ConstPropertyVisitor
{
    /** JSON writer. */
    JsonWriter& writer;

    /**
     * Write a property value.
     *
     * @param property The property metadata.
     * @param storage The property storage.
     */
    void write_value(
      const reflect::Property& property,
      const void* storage);

public:
    /**
     * Constructor.
     *
     * @param writer The JSON writer to use.
     */
    explicit JsonPropertyWriter(
      JsonWriter& writer);

    void visit(
      const reflect::Property& property,
      const void* value) override;
};

}    // namespace serial::json

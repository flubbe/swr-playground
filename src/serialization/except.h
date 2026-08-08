/**
 * Software Rasterizer Playground.
 *
 * Serialization errors.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <stdexcept>

namespace serial
{

/** A serialization error. */
class SerializationError
: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}    // namespace serial

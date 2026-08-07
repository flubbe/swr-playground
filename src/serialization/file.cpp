/**
 * Software Rasterizer Playground.
 *
 * Portable archive and serialization support.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <format>

#include "file.h"

namespace serial
{

FileArchive::FileArchive(
  fs::path path,
  bool read,
  bool write,
  std::endian target_byte_order)
: Archive{read, write, true, target_byte_order}
, path{std::move(path)}
{
    if(read && write)
    {
        throw SerializationError{
          std::format(
            "Cannot open file '{}' for reading and writing simultaneously.",
            this->path.string())};
    }

    if(write)
    {
        file.open(
          this->path,
          std::fstream::out | std::fstream::binary);
    }
    else if(read)
    {
        file.open(
          this->path,
          std::fstream::in | std::fstream::binary);
    }

    if(!file)
    {
        throw SerializationError{
          std::format(
            "Unable to open file '{}'.",
            this->path.string())};
    }
}

}    // namespace serial

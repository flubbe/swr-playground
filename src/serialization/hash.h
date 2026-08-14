/**
 * Software Rasterizer Playground.
 *
 * Hashing archive implementation.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "archive.h"
#include "hasher.h"

namespace serial
{

class HashArchive
: public Archive
{
    Hasher hasher;
    std::size_t processed_bytes;

public:
    HashArchive(
      std::uint64_t seed = 0)
    : Archive{true, false, false, std::endian::little}
    , hasher{seed}
    , processed_bytes{0}
    {
    }

    std::size_t tell() override
    {
        return processed_bytes;
    }

    [[noreturn]]
    std::size_t seek([[maybe_unused]] std::size_t pos) override
    {
        throw SerializationError{
          "Seek not supported for HashArchive"};
    }

    std::size_t size() override
    {
        return processed_bytes;
    }

    void serialize(
      std::span<std::byte> bytes) override
    {
        hasher.update(bytes);
        processed_bytes += bytes.size();
    }

    std::uint64_t digest() const
    {
        return hasher.digest();
    }
};

}    // namespace serial

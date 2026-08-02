/**
 * slang - a simple scripting language.
 *
 * in-memory archives.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2025
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "archive.h"

namespace serial
{

/** Archive for in-memory writes. */
class MemoryWriteArchive : public Archive
{
protected:
    /** The archive buffer. */
    swr::vector<std::byte> memory_buffer;

    void serialize_bytes(std::span<std::byte> bytes) override
    {
        memory_buffer.insert(memory_buffer.end(), bytes.begin(), bytes.end());
    }

public:
    /** Defaulted and deleted constructors. */
    MemoryWriteArchive() = delete;
    MemoryWriteArchive(const MemoryWriteArchive&) = default;
    MemoryWriteArchive(MemoryWriteArchive&&) = default;

    /** Default assignments. */
    MemoryWriteArchive& operator=(const MemoryWriteArchive&) = default;
    MemoryWriteArchive& operator=(MemoryWriteArchive&&) = default;

    /**
     * Construct an in-memory write archive.
     *
     * @param persistent Whether to mirror the behavior of persistent archive w.r.t. byte ordering.
     * @param byte_order The target byte order. Only relevant if `persistent` is `true`.
     */
    MemoryWriteArchive(
      bool persistent,
      std::endian byte_order = std::endian::native)
    : Archive{false, true, persistent, byte_order}
    {
    }

    std::size_t tell() override
    {
        return memory_buffer.size();
    }

    std::size_t seek([[maybe_unused]] std::size_t pos) override
    {
        throw SerializationError{
          "MemoryWriteArchive::seek: Operation not supported by archive."};
    }

    std::size_t size() override
    {
        return memory_buffer.size();
    }

    /** Clear the internal buffer. */
    void clear()
    {
        memory_buffer.clear();
    }

    /** Get the internal buffer. */
    const swr::vector<std::byte>& get_buffer() const
    {
        return memory_buffer;
    }
};

/** Archive for in-memory reads. */
class MemoryReadArchive : public Archive
{
protected:
    /** The archive's buffer reference. */
    const swr::vector<std::byte>& memory_buffer;

    /** Current buffer read offset. */
    std::size_t offset = 0;

    void serialize_bytes(std::span<std::byte> bytes) override
    {
        if(offset + bytes.size() > memory_buffer.size())
        {
            throw SerializationError{
              "MemoryReadArchive: read out of bounds."};
        }

        std::copy(
          memory_buffer.begin() + offset,
          memory_buffer.begin() + offset + bytes.size(),
          bytes.begin());
        offset += bytes.size();
    }

public:
    /** Defaulted and deleted constructors. */
    MemoryReadArchive() = delete;
    MemoryReadArchive(const MemoryReadArchive&) = default;
    MemoryReadArchive(MemoryReadArchive&&) = default;

    /** Deleted assignments. */
    MemoryReadArchive& operator=(const MemoryReadArchive&) = delete;
    MemoryReadArchive& operator=(MemoryReadArchive&&) = delete;

    /**
     * Construct an in-memory read archive.
     *
     * @param memory_buffer The underlying buffer.
     * @param persistent Whether to mirror the behavior of persistent archive w.r.t. byte ordering.
     * @param byte_order The target byte order. Only relevant if `persistent` is `true`.
     */
    MemoryReadArchive(
      const swr::vector<std::byte>& memory_buffer,
      bool persistent,
      std::endian byte_order = std::endian::native)
    : Archive{true, false, persistent, byte_order}
    , memory_buffer{memory_buffer}
    {
    }

    std::size_t tell() override
    {
        return offset;
    }

    std::size_t seek(std::size_t pos) override
    {
        if(pos >= memory_buffer.size())
        {
            throw SerializationError{
              "MemoryReadArchive::seek: position out of bounds."};
        }

        offset = pos;
        return offset;
    }

    std::size_t size() override
    {
        return memory_buffer.size();
    }

    /** Get the internal buffer. */
    const swr::vector<std::byte>& get_buffer() const
    {
        return memory_buffer;
    }
};

}    // namespace serial

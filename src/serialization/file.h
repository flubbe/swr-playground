/**
 * Software Rasterizer Playground.
 *
 * File read/write support for archives.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <fstream>
#include <filesystem>

#include "archive.h"

namespace fs = std::filesystem;

namespace serial
{

/** Base class for file readers and writers. */
class FileArchive : public Archive
{
protected:
    /** The file path. */
    fs::path path;

    /** The file stream. */
    std::fstream file;

public:
    /** Defaulted and deleted constructors. */
    FileArchive() = delete;
    FileArchive(const FileArchive&) = delete;
    FileArchive(FileArchive&&) = default;

    /** Default assignments. */
    FileArchive& operator=(const FileArchive&) = delete;
    FileArchive& operator=(FileArchive&&) = default;

    /**
     * Construct a `file_archive` from a path.
     *
     * @param path The file path.
     * @param read Whether the file is readable.
     * @param write Whether the file is writable.
     * @param target_byte_order The target byte order for persistent archives.
     */
    FileArchive(
      fs::path path,
      bool read,
      bool write,
      std::endian target_byte_order = std::endian::little);
};

/** A file writer. */
class FileWriteArchive : public FileArchive
{
protected:
    void serialize_bytes(std::span<std::byte> bytes) override
    {
        file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }

public:
    /** Defaulted and deleted constructors. */
    FileWriteArchive() = delete;
    FileWriteArchive(const FileWriteArchive&) = delete;
    FileWriteArchive(FileWriteArchive&&) = default;

    /** Assignments. */
    FileWriteArchive& operator=(const FileWriteArchive&) = delete;
    FileWriteArchive& operator=(FileWriteArchive&&) = default;

    /**
     * Open a file for writing.
     *
     * @param path The file path.
     * @param byte_order The archive's byte order. Defaults to endian::little.
     */
    FileWriteArchive(
      fs::path path,
      std::endian byte_order = std::endian::little)
    : FileArchive{std::move(path), false, true, byte_order}
    {
    }

    std::size_t tell() override
    {
        return static_cast<std::size_t>(file.tellp());
    }

    std::size_t seek(std::size_t pos) override
    {
        file.seekp(pos, std::ios::beg);
        return tell();
    }

    std::size_t size() override
    {
        // prefer fstream calls over filesystem calls to avoid running into buffer race conditions.
        auto cur = tell();
        auto beg = seek(0);

        file.seekp(0, std::ios::end);
        auto end = tell();

        seek(cur);

        return end - beg;
    }
};

/** A file reader. */
class FileReadArchive : public FileArchive
{
protected:
    void serialize_bytes(std::span<std::byte> bytes) override
    {
        file.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    }

public:
    /** Defaulted and deleted constructors. */
    FileReadArchive() = delete;
    FileReadArchive(const FileReadArchive&) = delete;
    FileReadArchive(FileReadArchive&&) = default;

    /** Assignments. */
    FileReadArchive& operator=(const FileReadArchive&) = delete;
    FileReadArchive& operator=(FileReadArchive&&) = default;

    /**
     * Open a file for reading.
     *
     * @param path The file path.
     * @param byte_order The archive's byte order. Defaults to endian::little.
     */
    FileReadArchive(
      fs::path path,
      std::endian byte_order = std::endian::little)
    : FileArchive{std::move(path), true, false, byte_order}
    {
    }

    std::size_t tell() override
    {
        return static_cast<std::size_t>(file.tellg());
    }

    std::size_t seek(std::size_t pos) override
    {
        file.seekg(pos, std::ios::beg);
        return tell();
    }

    std::size_t size() override
    {
        // prefer fstream calls over filesystem calls to avoid running into buffer race conditions.
        auto cur = tell();
        auto beg = seek(0);

        file.seekg(0, std::ios::end);
        auto end = tell();

        seek(cur);

        return end - beg;
    }
};

}    // namespace serial

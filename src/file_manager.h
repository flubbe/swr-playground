/**
 * Software Rasterizer Playground.
 *
 * File manager.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "containers/memory.h"
#include "serialization/file.h"

/** A file error. */
class FileError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/** A file manager, used for path resolution. */
class FileManager
{
    /** Search paths used to resolve relative read paths. */
    swr::vector<std::filesystem::path> search_paths;

    /** Root directory for relative write paths. */
    std::filesystem::path writable_root;

public:
    /** Default constructors. */
    FileManager() = default;
    FileManager(const FileManager&) = default;
    FileManager(FileManager&&) = default;

    /** Default assignments. */
    FileManager& operator=(const FileManager&) = default;
    FileManager& operator=(FileManager&&) = default;

    /**
     * Add a search path. Does nothing if the path already is in the search path list.
     * Does not check if the path actually exists.
     *
     * @param p The search path to add.
     */
    void add_search_path(
      std::filesystem::path p)
    {
        p = std::filesystem::canonical(p);
        if(std::ranges::find(search_paths, p) == search_paths.end())
        {
            search_paths.emplace_back(std::move(p));
        }
    }

    /**
     * Set the writable root directors.
     *
     * @param path The new writable root directory.
     * @throws Throws a `std::filesystem::filesystem_error` if the path does not exists.
     *     Throws a `FileError` if `path` is not a directory.
     */
    void set_writable_root(
      std::filesystem::path path)
    {
        path = std::filesystem::canonical(path);
        if(!std::filesystem::is_directory(path))
        {
            throw FileError{
              std::format(
                "Cannot set writable root. '{}' is not a directory.",
                path.string())};
        }

        writable_root = path;
    }

    /**
     * Check if a path exists. If the path is not an absolute path, the
     * path is checked within the search paths.
     */
    bool exists(
      const std::filesystem::path& p) const;

    /**
     * Check if a path represents a regular file. If the path is not an absolute path,
     * the path is checked within the search paths.
     */
    bool is_file(
      const std::filesystem::path& p) const;

    /**
     * Check if a path represents a directory. If the path is not an absolute path,
     * the path is checked within the search paths.
     */
    bool is_directory(
      const std::filesystem::path& p) const;

    /**
     * Resolve a file path for reading. If the path is relative, it is searched
     * for in the configured search paths.
     *
     * @param path The file path to resolve.
     * @returns The resolved path.
     * @throws A `FileError` if the path cannot be resolved.
     */
    [[nodiscard]]
    std::filesystem::path resolve_read(
      const std::filesystem::path& path) const;

    /**
     * Resolve a file path for writing. If the path is relative, it is resolved
     * relative to the configured writable root.
     *
     * @param path The file path to resolve.
     * @returns The resolved path.
     * @throws A `FileError` if the path cannot be resolved.
     */
    [[nodiscard]]
    std::filesystem::path resolve_write(
      const std::filesystem::path& path) const;

    /**
     * Open a file for reading using an archive. If the file path is not an absolute path,
     * the path is checked within the search paths.
     *
     * Files are opened as little endian archives by default.
     *
     * @param path The file path.
     * @param target_byte_order File byte order. Defaults to little endian.
     * @returns A readable file archive.
     * @throws A `FileError` if the path cannot be opened.
     */
    swr::unique_ptr<serial::FileReadArchive> open_read(
      const std::filesystem::path& path,
      std::endian target_byte_order = std::endian::little) const;

    /**
     * Open a file for writing using an archive.
     *
     * The path is interpreted directly and is not searched using the configured
     * search paths. Relative paths are resolved relative to the configured
     * writable root.
     *
     * Files are opened as little endian archives by default.
     *
     * @param path The file path.
     * @param target_byte_order File byte order. Defaults to little endian.
     * @returns A writable file archive.
     * @throws A `FileError` if the path cannot be opened.
     */
    swr::unique_ptr<serial::FileWriteArchive> open_write(
      const std::filesystem::path& path,
      std::endian target_byte_order = std::endian::little) const;
};

/**
 * Software Rasterizer Playground.
 *
 * File manager.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <format>

#include "containers/memory.h"
#include "file_manager.h"

namespace fs = std::filesystem;

using serial::FileReadArchive;
using serial::FileWriteArchive;

bool FileManager::exists(
  const fs::path& path) const
{
    if(path.is_absolute())
    {
        return fs::exists(path);
    }

    return std::ranges::any_of(
      search_paths,
      [&path](const auto& sp) -> bool
      {
          return fs::exists(sp / path);
      });
}

bool FileManager::is_file(
  const fs::path& path) const
{
    if(path.is_absolute())
    {
        return fs::is_regular_file(path);
    }

    return std::ranges::any_of(
      search_paths,
      [&path](const auto& sp) -> bool
      {
          return fs::is_regular_file(sp / path);
      });
}

bool FileManager::is_directory(
  const fs::path& path) const
{
    if(path.is_absolute())
    {
        return fs::is_directory(path);
    }

    return std::ranges::any_of(
      search_paths,
      [&path](const auto& sp) -> bool
      {
          return fs::is_directory(sp / path);
      });
}

fs::path FileManager::resolve_read(
  const fs::path& path) const
{
    if(path.is_absolute())
    {
        if(!fs::is_regular_file(path))
        {
            throw FileError{
              std::format(
                "Resolved path '{}' is not a file.",
                path.string())};
        }
        return fs::canonical(path);
    }

    for(const auto& sp: search_paths)
    {
        if(fs::is_regular_file(sp / path))
        {
            return fs::canonical(sp / path);
        }
    }

    throw FileError{
      std::format(
        "Unable to resolve path '{}'.",
        path.string())};
}

fs::path FileManager::resolve_write(
  const fs::path& path) const
{
    if(path.is_absolute())
    {
        return path.lexically_normal();
    }

    return (writable_root / path).lexically_normal();
}

swr::unique_ptr<FileReadArchive> FileManager::open_read(
  const fs::path& path,
  std::endian target_byte_order) const
{
    const fs::path resolved_path = resolve_read(path);
    return swr::make_unique<FileReadArchive>(
      resolved_path,
      target_byte_order);
}

swr::unique_ptr<FileWriteArchive> FileManager::open_write(
  const fs::path& path,
  std::endian target_byte_order) const
{
    const fs::path resolved_path = resolve_write(path);
    return swr::make_unique<FileWriteArchive>(
      resolved_path,
      target_byte_order);
}

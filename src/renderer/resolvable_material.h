/**
 * Software Rasterizer Playground.
 *
 * A material that is asynchronously resolved.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cassert>

#include "containers/memory.h"
#include "containers/string.h"

/*
 * Forward declarations.
 */
struct MaterialEntry;

/** A material that is asynchronously resolved. */
class ResolvableMaterial
{
    friend class MaterialManager;

    /** Asset path identifying the material. */
    swr::string path;

    /** The material entry, containing resources and handles. */
    swr::shared_ptr<MaterialEntry> entry;

public:
    /** Deleted default constructor. */
    ResolvableMaterial() = delete;

    /** Defaulted copy/moves. */
    ResolvableMaterial(const ResolvableMaterial&) = default;
    ResolvableMaterial(ResolvableMaterial&&) = default;

    /**
     * Constructor.
     *
     * @param path Path identifying the material.
     * @param entry The material entry.
     */
    explicit ResolvableMaterial(
      std::string_view path,
      swr::shared_ptr<MaterialEntry> entry)
    : path{path}
    , entry{std::move(entry)}
    {
    }

    ResolvableMaterial& operator=(const ResolvableMaterial&) = default;
    ResolvableMaterial& operator=(ResolvableMaterial&&) = default;

    MaterialEntry* operator->() noexcept
    {
        return entry.get();
    }

    const MaterialEntry* operator->() const noexcept
    {
        return entry.get();
    }

    MaterialEntry& operator*() noexcept
    {
        return *entry;
    }

    const MaterialEntry& operator*() const noexcept
    {
        return *entry;
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(entry);
    }

    /** Get the path identifying this material. */
    const swr::string& get_path() const
    {
        return path;
    }
};

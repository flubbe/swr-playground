/**
 * Software Rasterizer Playground.
 *
 * A material that is potentially asynchronously resolved.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "containers/memory.h"
#include "containers/string.h"
#include "types.h"

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

    /*
     * TODO unify these into a variant.
     */

    /** The async material entry, containing resources and handles. */
    swr::shared_ptr<MaterialEntry> entry;

    /** The directly loaded handle. */
    MaterialHandle handle;

public:
    /** Deleted default constructor. */
    ResolvableMaterial() = delete;

    /** Defaulted copy/moves. */
    ResolvableMaterial(const ResolvableMaterial&) = default;
    ResolvableMaterial(ResolvableMaterial&&) = default;

    /**
     * Constructor for async material loading.
     *
     * @param path Path identifying the material.
     * @param entry The material entry.
     */
    explicit ResolvableMaterial(
      std::string_view path,
      swr::shared_ptr<MaterialEntry> entry)
    : path{path}
    , entry{std::move(entry)}
    , handle{0}
    {
    }

    /**
     * Direct material construction.
     *
     * @param path Path identifying the material.
     * @param handle The material handle.
     */
    explicit ResolvableMaterial(
      std::string_view path,
      MaterialHandle handle)
    : path{path}
    , entry{}
    , handle{handle}
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

    /** Get the material handle. */
    std::optional<MaterialHandle> try_get();
};

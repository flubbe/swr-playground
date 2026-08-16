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

/** A material that is potentially asynchronously resolved. */
class ResolvableMaterial
{
    /** Asset path identifying the material. */
    swr::string path;

    /** Async or directly loaded material. */
    std::variant<
      swr::shared_ptr<MaterialEntry>,
      MaterialHandle>
      material;

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
    , material{std::move(entry)}
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
    , material{handle}
    {
    }

    ResolvableMaterial& operator=(const ResolvableMaterial&) = default;
    ResolvableMaterial& operator=(ResolvableMaterial&&) = default;

    explicit operator bool() const noexcept
    {
        if(auto* result =
             std::get_if<swr::shared_ptr<MaterialEntry>>(&material))
        {
            return static_cast<bool>(*result);
        }

        return std::get<MaterialHandle>(material) != 0;
    }

    /** Whether the material is resolved. */
    bool is_resolved() const;

    /** Get the material handle. */
    std::optional<MaterialHandle> try_get() const;

    /** Get the path identifying this material. */
    const swr::string& get_path() const
    {
        return path;
    }

    /**
     * Get the `MaterialEntry`.
     *
     * @throws Throws `std::bad_variant_access` if the resolvable materials
     *     holds a material handle.
     */
    [[nodiscard]]
    MaterialEntry& get_entry()
    {
        return *std::get<swr::shared_ptr<MaterialEntry>>(material);
    }
};

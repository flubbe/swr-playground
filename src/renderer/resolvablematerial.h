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

/*
 * Forward declarations.
 */
struct MaterialEntry;

/** A material that is asynchronously resolved. */
class ResolvableMaterial
{
    friend class MaterialManager;

    /** The material entry, containing resources and handles. */
    swr::shared_ptr<MaterialEntry> entry;

    /**
     * Constructor.
     *
     * @param entry The material entry.
     */
    explicit ResolvableMaterial(
      swr::shared_ptr<MaterialEntry> entry)
    : entry{std::move(entry)}
    {
        assert(this->entry != nullptr);
    }

public:
    /** Deleted default constructor. */
    ResolvableMaterial() = delete;

    /** Defaulted copy/moves. */
    ResolvableMaterial(const ResolvableMaterial&) = default;
    ResolvableMaterial(ResolvableMaterial&&) = default;

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
};

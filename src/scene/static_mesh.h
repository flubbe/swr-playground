/**
 * Software Rasterizer Playground.
 *
 * Static mesh object.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <utility>
#include <vector>

#include "render_types.h"
#include "object.h"

class StaticMesh
: public reflect::Reflected<StaticMesh, Object>
{
    std::vector<MeshSection> mesh_sections;

public:
    static void register_properties(reflect::ClassInfo& class_info);

    StaticMesh() = default;

    explicit StaticMesh(std::vector<MeshSection> sections)
    : mesh_sections{std::move(sections)}
    {
    }

    void set_mesh_sections(std::vector<MeshSection> sections)
    {
        mesh_sections = std::move(sections);
    }

    void clear_mesh_sections()
    {
        mesh_sections.clear();
    }

    [[nodiscard]]
    const std::vector<MeshSection>& get_mesh_sections() const
    {
        return mesh_sections;
    }

    [[nodiscard]]
    bool has_mesh_sections() const noexcept
    {
        return !mesh_sections.empty();
    }
};

DECLARE_REFLECTION(Scene, StaticMesh);

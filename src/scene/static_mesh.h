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

#include <algorithm>
#include <cstddef>

#include "containers/vector.h"
#include "meshes/mesh.h"
#include "render_types.h"
#include "object.h"

/** One renderable level of detail for a static mesh. */
struct StaticMeshLod
{
    /** Sections to draw when this LOD is selected. */
    swr::vector<MeshSection> mesh_sections;

    /** Minimum projected screen-height fraction required to select this LOD. */
    float min_screen_height{0.f};

    /** Combined local-space bounds for all sections in this LOD. */
    MeshBounds bounds;
};

class StaticMesh
: public reflect::Reflected<StaticMesh, Object>
{
    swr::vector<StaticMeshLod> mesh_lods;
    MeshBounds mesh_bounds;

    void update_bounds() noexcept;

public:
    static void register_properties(reflect::ClassInfo& class_info);

    /** Whether this mesh contributes to shadow maps when supported by the renderer. */
    bool casts_shadows{false};

    /** Whether this mesh receives shadows when supported by the renderer. */
    bool receives_shadows{true};

    StaticMesh() = default;

    explicit StaticMesh(swr::vector<MeshSection> sections);

    StaticMesh(
      swr::vector<MeshSection> sections,
      MeshBounds bounds);

    explicit StaticMesh(swr::vector<StaticMeshLod> lods);

    void set_mesh_sections(swr::vector<MeshSection> sections);

    void set_mesh_sections(
      swr::vector<MeshSection> sections,
      MeshBounds bounds);

    void set_lods(swr::vector<StaticMeshLod> lods);

    void clear_mesh_sections() noexcept;

    [[nodiscard]]
    const swr::vector<MeshSection>& get_mesh_sections() const
    {
        static const swr::vector<MeshSection> empty;
        if(mesh_lods.empty())
        {
            return empty;
        }

        return mesh_lods.front().mesh_sections;
    }

    [[nodiscard]]
    const swr::vector<StaticMeshLod>& get_lods() const
    {
        return mesh_lods;
    }

    /*
     * FIXME This is currently here because the LOD's materials should be
     *       changable at run-time.
     */
    [[nodiscard]]
    swr::vector<StaticMeshLod>& get_lods()
    {
        return mesh_lods;
    }

    [[nodiscard]]
    const MeshBounds& get_bounds() const noexcept
    {
        return mesh_bounds;
    }

    [[nodiscard]]
    const StaticMeshLod& get_lod(std::size_t index) const
    {
        return mesh_lods[index];
    }

    [[nodiscard]]
    std::size_t get_lod_count() const noexcept
    {
        return mesh_lods.size();
    }

    [[nodiscard]]
    bool has_mesh_sections() const noexcept
    {
        return std::ranges::any_of(
          mesh_lods,
          [](const StaticMeshLod& lod)
          {
              return !lod.mesh_sections.empty();
          });
    }

    [[nodiscard]]
    std::size_t select_lod(float screen_height_fraction) const noexcept;
};

DECLARE_REFLECTION(Scene, StaticMesh);

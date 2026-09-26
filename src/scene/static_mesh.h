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
#include <functional>
#include <optional>

#include "containers/vector.h"
#include "meshes/mesh.h"
#include "renderer/mesh.h"
#include "renderer/mesh_section.h"
#include "object.h"

/*
 * Forward declarations.
 */

struct AssetResolver;
class MaterialManager;
class MeshManager;
class RenderDevice;
class Scene;

/** A static mesh. */
class StaticMesh
: public reflect::Reflected<StaticMesh, Object>
{
protected:
    assets::AssetPath path;
    swr::vector<assets::AssetPath> materials;
    std::optional<MaterialRef> material_ref;

    swr::vector<MeshSection> sections;
    std::optional<MeshRef> mesh_ref;
    std::optional<MeshRef> pending_mesh_ref;

    MeshBounds bounds;

    bool mesh_dirty{false};

    void update_bounds() noexcept;

public:
    /** Property registration hook. */
    static void register_properties(
      reflect::ClassInfo& class_info);

    /** Whether this mesh contributes to shadow maps when supported by the renderer. */
    bool casts_shadows{false};

    /** Whether this mesh receives shadows when supported by the renderer. */
    bool receives_shadows{true};

    StaticMesh() = default;

    void resolve(AssetResolver& resolver) override;
    void post_load() override;
    void release() override;
    void on_properties_changed() override;

    void init(
      const assets::AssetPath& path,
      const swr::vector<assets::AssetPath>& materials,
      MeshRef mesh);
    void init(
      const assets::AssetPath& path,
      const swr::vector<assets::AssetPath>& materials,
      swr::vector<MeshSection> sections);

    void set_sections(
      swr::vector<MeshSection> sections);

    void set_mesh_ref(MeshRef ref)
    {
        pending_mesh_ref.reset();
        mesh_ref = std::move(ref);
    }

    void set_pending_mesh_ref(MeshRef ref)
    {
        pending_mesh_ref = std::move(ref);
    }

    void clear_pending_mesh_ref()
    {
        pending_mesh_ref.reset();
    }

    [[nodiscard]]
    const std::optional<MeshRef>& get_pending_mesh_ref() const noexcept
    {
        return pending_mesh_ref;
    }

    /**
     * Marks the mesh as dirty. If the mesh is part of a `Scene`,
     * the mesh is added to the `Scene`'s dirty list.
     */
    void mark_mesh_dirty();

    /** Clear the mesh dirty flag. Does not modify the `Scene`'s dirty list. */
    void clear_mesh_dirty();

    /** Return whether this mesh is marked as dirty. */
    [[nodiscard]]
    bool is_mesh_dirty() const noexcept
    {
        return mesh_dirty;
    }

    [[nodiscard]]
    const assets::AssetPath& get_path() const
    {
        return path;
    }

    [[nodiscard]]
    const swr::vector<assets::AssetPath>& get_material_paths() const
    {
        return materials;
    }

    [[nodiscard]]
    const std::optional<MaterialRef>& get_material_ref() const
    {
        return material_ref;
    }

    [[nodiscard]]
    const swr::vector<MeshSection>& get_sections() const
    {
        return sections;
    }

    /*
     * FIXME This is currently here because the LOD's materials should be
     *       changable at run-time.
     */
    [[nodiscard]]
    swr::vector<MeshSection>& get_sections()
    {
        return sections;
    }

    [[nodiscard]]
    MeshBounds get_bounds() const noexcept
    {
        return bounds;
    }

    [[nodiscard]]
    bool has_mesh_sections() const noexcept
    {
        return !sections.empty();
    }
};

DECLARE_REFLECTION(Scene, StaticMesh);

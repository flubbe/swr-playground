/**
 * Software Rasterizer Playground.
 *
 * Create gear geometry.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include "static_mesh.h"

/*
 * Forward declarations.
 */

class RenderDevice;

struct GearGeometry
{
    std::vector<ml::vec4> inner_vertices;
    std::vector<ml::vec4> inner_normals;
    std::vector<std::uint32_t> inner_indices;

    std::vector<ml::vec4> outer_vertices;
    std::vector<ml::vec4> outer_normals;
    std::vector<std::uint32_t> outer_indices;
};

struct gear_limits
{
    static constexpr float min_outer_radius = 0.01f;
    static constexpr float max_outer_radius = 100.f;
    static constexpr float min_inner_radius = 0.0f;
    static constexpr float max_inner_radius = 100.f;
    static constexpr float min_width = 0.01f;
    static constexpr float max_width = 100.f;
    static constexpr float radius_epsilon = 0.001f;
    static constexpr float depth_epsilon = 0.001f;
    static constexpr int min_teeth = 5;
    static constexpr int max_teeth = 200;
    static constexpr float min_tooth_depth = 0.01f;
    static constexpr float max_tooth_depth = 10.0f;
};

GearGeometry make_gear(
  float inner_radius,
  float outer_radius,
  float width,
  int teeth,
  float tooth_depth);

struct GearParameters
{
    swr::vector<assets::AssetPath> materials;
    MeshRef mesh;
    MeshBounds bounds;
    float inner_radius{1.0f};
    float outer_radius{2.0f};
    float width{1.0f};
    int teeth{10};
    float tooth_depth{0.7f};
    ml::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};
};

/** A gear object. */
class Gear
: public reflect::Reflected<Gear, StaticMesh>
{
    float inner_radius{1.0f};
    float outer_radius{2.0f};
    float width{1.0f};
    int teeth{10};
    float tooth_depth{0.7f};

    /** Gear color, gray by default. */
    ml::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};

public:
    /** Property registration hook. */
    static void register_properties(reflect::ClassInfo& class_info);

    Gear() = default;

    void post_load() override;

    void init(const GearParameters& params);

    void clamp_runtime_parameters() noexcept;

    GearGeometry generate_mesh() const;

    [[nodiscard]]
    float get_inner_radius() const noexcept
    {
        return inner_radius;
    }

    [[nodiscard]]
    float get_outer_radius() const noexcept
    {
        return outer_radius;
    }

    [[nodiscard]]
    float get_width() const noexcept
    {
        return width;
    }

    [[nodiscard]]
    int get_teeth() const noexcept
    {
        return teeth;
    }

    [[nodiscard]]
    float get_tooth_depth() const noexcept
    {
        return tooth_depth;
    }

    [[nodiscard]]
    ml::vec4 get_color() const noexcept
    {
        return color;
    }

    /**
     * Create the resources to create a gear mesh.
     *
     * @param mesh_manager Mesh manager.
     * @param name Gear name. Must be unique per name. Used for mesh identification.
     * @param material Material.
     * @param inner_radius Inner gear radius.
     * @param outer_radius Outer gear radius.
     * @param width Gear width.
     * @param teeth Teeth count.
     * @param color Color.
     * @param geom Gear geometry.
     * @returns Returns the gear parameters, including a mesh reference.
     */
    static GearParameters create_gear_resources(
      MeshManager& mesh_manager,
      const swr::string& name,
      MaterialRef material,
      float inner_radius,
      float outer_radius,
      float width,
      int teeth,
      float tooth_depth,
      const ml::vec4& color,
      const GearGeometry& geom);
};

DECLARE_REFLECTION(Scene, Gear);

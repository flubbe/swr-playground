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

#include "object.h"
#include "shader.h"

struct GearGeometry
{
    std::vector<ml::vec4> inner_vertices;
    std::vector<ml::vec4> inner_normals;
    std::vector<std::uint32_t> inner_indices;

    std::vector<ml::vec4> outer_vertices;
    std::vector<ml::vec4> outer_normals;
    std::vector<std::uint32_t> outer_indices;
};

GearGeometry make_gear(
  float inner_radius,
  float outer_radius,
  float width,
  int teeth,
  float tooth_depth);

struct GearParameters
{
    RenderData inner;
    RenderData outer;
    float inner_radius{1.0f};
    float outer_radius{2.0f};
    float width{1.0f};
    int teeth{10};
    float tooth_depth{0.7f};
};

/** A gear object. */
class Gear : public reflect::Reflected<Gear, Object>
{
    float inner_radius{1.0f};
    float outer_radius{2.0f};
    float width{1.0f};
    int teeth{10};
    float tooth_depth{0.7f};
    float built_inner_radius{1.0f};
    float built_outer_radius{2.0f};
    float built_width{1.0f};
    int built_teeth{10};
    float built_tooth_depth{0.7f};

public:
    static void register_properties(reflect::ClassInfo& class_info);

    Gear()
    : Reflected<Gear, Object>{Gear::static_class()}
    {
    }

    explicit Gear(const GearParameters& params);

    [[nodiscard]]
    bool needs_rebuild() const noexcept;

    void mark_rebuilt() noexcept;

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
};

DECLARE_REFLECTION(Scene, Gear);

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
};

/** A gear object. */
class Gear : public reflect::Reflected<Gear, Object>
{
public:
    static void register_properties(reflect::ClassInfo& class_info);

    Gear()
    : Reflected<Gear, Object>{Gear::static_class()}
    {
    }

    explicit Gear(const GearParameters& params);
};

DECLARE_REFLECTION(Scene, Gear);

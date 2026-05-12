/**
 * Software Rasterizer Playground.
 *
 * Create gear geometry.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <numbers>
#include <cmath>
#include <algorithm>

#include "reflection/builtin_properties.h"
#include "gear.h"
#include "renderdevice.h"

/** create a gear and upload it to the graphics driver. the code here is adapted from glxgears.c. */
GearGeometry make_gear(
  float inner_radius,
  float outer_radius,
  float width,
  int teeth,
  float tooth_depth)
{
    teeth = std::clamp(
      teeth,
      gear_limits::min_teeth,
      gear_limits::max_teeth);
    tooth_depth = std::clamp(
      tooth_depth,
      gear_limits::min_tooth_depth,
      gear_limits::max_tooth_depth);

    GearGeometry gear_geom;

    float r0 = inner_radius;
    float r1 = outer_radius - (tooth_depth / 2.f);
    float r2 = outer_radius + (tooth_depth / 2.f);

    float da = 2.f * std::numbers::pi_v<float> / static_cast<float>(teeth) / 4.f;

    std::vector<ml::vec4> vb;
    std::vector<ml::vec4> nb;
    std::vector<std::uint32_t> ib;
    const std::size_t teeth_count = static_cast<std::size_t>(teeth);

    // Outer geometry exact capacities:
    // vertices/normals = (4t + 2) front/back faces + 8t front/back tooth sides + 16t outward quads
    //                  = 26t + 2
    // indices          = 12t front/back faces + 12t front/back tooth sides + 36t outward quads
    //                  = 60t
    vb.reserve(26 * teeth_count + 2);
    nb.reserve(26 * teeth_count + 2);
    ib.reserve(60 * teeth_count);

    /* draw front face */
    for(int i = 0; i <= teeth; ++i)
    {
        float angle = static_cast<float>(i) * 2.f * std::numbers::pi_v<float> / static_cast<float>(teeth);
        vb.emplace_back(r0 * std::cos(angle), r0 * std::sin(angle), width * 0.5f);
        vb.emplace_back(r1 * std::cos(angle), r1 * std::sin(angle), width * 0.5f);

        nb.emplace_back(0, 0, 1, 0);
        nb.emplace_back(0, 0, 1, 0);

        if(i != 0)
        {
            auto cur_idx = vb.size() - 1;
            ib.emplace_back(cur_idx - 1);
            ib.emplace_back(cur_idx - 3);
            ib.emplace_back(cur_idx - 2);

            ib.emplace_back(cur_idx - 1);
            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx);
        }

        if(i < teeth)
        {
            vb.emplace_back(r0 * std::cos(angle), r0 * std::sin(angle), width * 0.5f);
            vb.emplace_back(r1 * std::cos(angle + 3 * da), r1 * std::sin(angle + 3 * da), width * 0.5f);

            nb.emplace_back(0, 0, 1, 0);
            nb.emplace_back(0, 0, 1, 0);

            auto cur_idx = vb.size() - 1;
            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx - 1);
            ib.emplace_back(cur_idx - 3);

            ib.emplace_back(cur_idx - 1);
            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx);
        }
    }

    /* draw front sides of teeth */
    da = 2.f * std::numbers::pi_v<float> / static_cast<float>(teeth) / 4.f;
    for(int i = 0; i < teeth; ++i)
    {
        float angle = static_cast<float>(i) * 2.f * std::numbers::pi_v<float> / static_cast<float>(teeth);

        vb.emplace_back(r1 * std::cos(angle), r1 * std::sin(angle), width * 0.5f);
        vb.emplace_back(r2 * std::cos(angle + da), r2 * std::sin(angle + da), width * 0.5f);
        vb.emplace_back(r2 * std::cos(angle + 2 * da), r2 * std::sin(angle + 2 * da), width * 0.5f);
        vb.emplace_back(r1 * std::cos(angle + 3 * da), r1 * std::sin(angle + 3 * da), width * 0.5f);

        nb.emplace_back(0, 0, 1, 0);
        nb.emplace_back(0, 0, 1, 0);
        nb.emplace_back(0, 0, 1, 0);
        nb.emplace_back(0, 0, 1, 0);

        auto cur_idx = vb.size() - 1;
        ib.emplace_back(cur_idx - 3);
        ib.emplace_back(cur_idx - 2);
        ib.emplace_back(cur_idx - 1);

        ib.emplace_back(cur_idx - 3);
        ib.emplace_back(cur_idx - 1);
        ib.emplace_back(cur_idx);
    }

    /* draw back face */
    for(int i = 0; i <= teeth; ++i)
    {
        float angle = static_cast<float>(i) * 2.f * std::numbers::pi_v<float> / static_cast<float>(teeth);
        vb.emplace_back(r1 * std::cos(angle), r1 * std::sin(angle), -width * 0.5f);
        vb.emplace_back(r0 * std::cos(angle), r0 * std::sin(angle), -width * 0.5f);

        nb.emplace_back(0, 0, -1, 0);
        nb.emplace_back(0, 0, -1, 0);

        if(i != 0)
        {
            auto cur_idx = vb.size() - 1;
            ib.emplace_back(cur_idx - 3);
            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx - 1);

            ib.emplace_back(cur_idx - 1);
            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx);
        }

        if(i < teeth)
        {
            vb.emplace_back(r1 * std::cos(angle + 3 * da), r1 * std::sin(angle + 3 * da), -width * 0.5f);
            vb.emplace_back(r0 * std::cos(angle), r0 * std::sin(angle), -width * 0.5f);

            nb.emplace_back(0, 0, -1, 0);
            nb.emplace_back(0, 0, -1, 0);

            auto cur_idx = vb.size() - 1;
            ib.emplace_back(cur_idx - 3);
            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx - 1);

            ib.emplace_back(cur_idx - 1);
            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx);
        }
    }

    /* draw back sides of teeth */
    da = 2.f * std::numbers::pi_v<float> / static_cast<float>(teeth) / 4.f;
    for(int i = 0; i < teeth; ++i)
    {
        float angle = static_cast<float>(i) * 2.f * std::numbers::pi_v<float> / static_cast<float>(teeth);

        vb.emplace_back(r1 * std::cos(angle + 3 * da), r1 * std::sin(angle + 3 * da), -width * 0.5f);
        vb.emplace_back(r2 * std::cos(angle + 2 * da), r2 * std::sin(angle + 2 * da), -width * 0.5f);
        vb.emplace_back(r2 * std::cos(angle + da), r2 * std::sin(angle + da), -width * 0.5f);
        vb.emplace_back(r1 * std::cos(angle), r1 * std::sin(angle), -width * 0.5f);

        nb.emplace_back(0, 0, -1, 0);
        nb.emplace_back(0, 0, -1, 0);
        nb.emplace_back(0, 0, -1, 0);
        nb.emplace_back(0, 0, -1, 0);

        auto cur_idx = vb.size() - 1;
        ib.emplace_back(cur_idx - 3);
        ib.emplace_back(cur_idx - 2);
        ib.emplace_back(cur_idx - 1);

        ib.emplace_back(cur_idx - 3);
        ib.emplace_back(cur_idx - 1);
        ib.emplace_back(cur_idx);
    }

    const auto safe_normal_xy = [](float x, float y) -> ml::vec4
    {
        const float len2 = x * x + y * y;
        if(len2 <= 1e-12f)
        {
            return {1.f, 0.f, 0.f, 0.f};
        }
        const float inv_len = 1.0f / std::sqrt(len2);
        return {x * inv_len, y * inv_len, 0.f, 0.f};
    };

    const auto append_quad = [&vb, &nb, &ib](
                               const ml::vec4& p0,
                               const ml::vec4& p1,
                               const ml::vec4& p2,
                               const ml::vec4& p3,
                               const ml::vec4& normal)
    {
        const std::uint32_t base = static_cast<std::uint32_t>(vb.size());
        vb.emplace_back(p0);
        vb.emplace_back(p1);
        vb.emplace_back(p2);
        vb.emplace_back(p3);
        nb.emplace_back(normal);
        nb.emplace_back(normal);
        nb.emplace_back(normal);
        nb.emplace_back(normal);

        ib.emplace_back(base + 0);
        ib.emplace_back(base + 1);
        ib.emplace_back(base + 2);
        ib.emplace_back(base + 0);
        ib.emplace_back(base + 2);
        ib.emplace_back(base + 3);
    };

    /* draw outward faces of teeth with explicit per-face quads */
    const float tooth_pitch =
      2.f * std::numbers::pi_v<float> / static_cast<float>(teeth);
    for(int i = 0; i < teeth; ++i)
    {
        const float angle =
          static_cast<float>(i) * tooth_pitch;
        const float next_angle = angle + tooth_pitch;

        const float a0 = angle;
        const float a1 = angle + da;
        const float a2 = angle + 2.f * da;
        const float a3 = angle + 3.f * da;

        const ml::vec4 p0f{r1 * std::cos(a0), r1 * std::sin(a0), width * 0.5f};
        const ml::vec4 p0b{r1 * std::cos(a0), r1 * std::sin(a0), -width * 0.5f};
        const ml::vec4 p1f{r2 * std::cos(a1), r2 * std::sin(a1), width * 0.5f};
        const ml::vec4 p1b{r2 * std::cos(a1), r2 * std::sin(a1), -width * 0.5f};
        const ml::vec4 p2f{r2 * std::cos(a2), r2 * std::sin(a2), width * 0.5f};
        const ml::vec4 p2b{r2 * std::cos(a2), r2 * std::sin(a2), -width * 0.5f};
        const ml::vec4 p3f{r1 * std::cos(a3), r1 * std::sin(a3), width * 0.5f};
        const ml::vec4 p3b{r1 * std::cos(a3), r1 * std::sin(a3), -width * 0.5f};
        const ml::vec4 p4f{r1 * std::cos(next_angle), r1 * std::sin(next_angle), width * 0.5f};
        const ml::vec4 p4b{r1 * std::cos(next_angle), r1 * std::sin(next_angle), -width * 0.5f};

        const ml::vec4 n01 = safe_normal_xy(
          p1f.y - p0f.y,
          -(p1f.x - p0f.x));
        const ml::vec4 n23 = safe_normal_xy(
          p3f.y - p2f.y,
          -(p3f.x - p2f.x));
        const ml::vec4 nr0 = safe_normal_xy(std::cos(angle), std::sin(angle));

        append_quad(p0f, p0b, p1b, p1f, n01);
        append_quad(p1f, p1b, p2b, p2f, nr0);
        append_quad(p2f, p2b, p3b, p3f, n23);
        append_quad(p3f, p3b, p4b, p4f, nr0);
    }

    /* create outside of the gear. */
    gear_geom.outer_vertices = std::move(vb);
    gear_geom.outer_normals = std::move(nb);
    gear_geom.outer_indices = std::move(ib);

    /* clear buffers for the inner cylinder. */
    vb.clear();
    nb.clear();
    ib.clear();
    vb.reserve(2 * (teeth_count + 1));
    nb.reserve(2 * (teeth_count + 1));
    ib.reserve(6 * teeth_count);

    /* draw inside radius cylinder */
    for(int i = 0; i <= teeth; i++)
    {
        float angle = static_cast<float>(i) * 2.f * std::numbers::pi_v<float> / static_cast<float>(teeth);
        vb.emplace_back(r0 * std::cos(angle), r0 * std::sin(angle), -width * 0.5f);
        vb.emplace_back(r0 * std::cos(angle), r0 * std::sin(angle), width * 0.5f);

        nb.emplace_back(-std::cos(angle), -std::sin(angle), 0, 0);
        nb.emplace_back(-std::cos(angle), -std::sin(angle), 0, 0);

        if(i != 0)
        {
            auto cur_idx = vb.size() - 1;
            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx - 1);
            ib.emplace_back(cur_idx - 3);

            ib.emplace_back(cur_idx - 2);
            ib.emplace_back(cur_idx);
            ib.emplace_back(cur_idx - 1);
        }
    }

    /* create inner cylinder. */
    gear_geom.inner_vertices = std::move(vb);
    gear_geom.inner_normals = std::move(nb);
    gear_geom.inner_indices = std::move(ib);

    return gear_geom;
}

/*
 * Gear object.
 */

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Gear);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

Gear::Gear(
  const GearParameters& params)
: Reflected<Gear, Object>{
    std::vector{
      RenderData{
        .mesh_handle = params.inner.mesh_handle,
        .material_handle = params.inner.material_handle},
      RenderData{
        .mesh_handle = params.outer.mesh_handle,
        .material_handle = params.outer.material_handle}}}
, inner_radius{params.inner_radius}
, outer_radius{params.outer_radius}
, width{params.width}
, teeth{params.teeth}
, tooth_depth{params.tooth_depth}
, built_inner_radius{params.inner_radius}
, built_outer_radius{params.outer_radius}
, built_width{params.width}
, built_teeth{params.teeth}
, built_tooth_depth{params.tooth_depth}
{
    clamp_runtime_parameters();
    built_teeth = teeth;
    built_tooth_depth = tooth_depth;
}

void Gear::clamp_runtime_parameters() noexcept
{
    outer_radius = std::max(
      outer_radius,
      gear_limits::min_outer_radius);
    inner_radius = std::clamp(
      inner_radius,
      gear_limits::min_inner_radius,
      std::max(
        gear_limits::min_inner_radius,
        outer_radius - gear_limits::radius_epsilon));
    width = std::max(width, gear_limits::min_width);

    const float max_depth_from_outer = std::max(
      gear_limits::min_tooth_depth,
      2.0f * (outer_radius - gear_limits::depth_epsilon));
    const float max_depth_from_inner = std::max(
      gear_limits::min_tooth_depth,
      2.0f * (outer_radius - inner_radius - gear_limits::depth_epsilon));
    const float max_allowed_depth = std::min(
      {gear_limits::max_tooth_depth,
       max_depth_from_outer,
       max_depth_from_inner});

    teeth = std::clamp(
      teeth,
      gear_limits::min_teeth,
      gear_limits::max_teeth);
    tooth_depth = std::clamp(
      tooth_depth,
      gear_limits::min_tooth_depth,
      max_allowed_depth);
}

bool Gear::needs_rebuild() const noexcept
{
    const auto changed_float = [](float a, float b) noexcept
    {
        return std::fabs(a - b) > 1e-6f;
    };

    return changed_float(inner_radius, built_inner_radius)
           || changed_float(outer_radius, built_outer_radius)
           || changed_float(width, built_width)
           || teeth != built_teeth
           || changed_float(tooth_depth, built_tooth_depth);
}

void Gear::mark_rebuilt() noexcept
{
    built_inner_radius = inner_radius;
    built_outer_radius = outer_radius;
    built_width = width;
    built_teeth = teeth;
    built_tooth_depth = tooth_depth;
}

void Gear::register_properties(
  reflect::ClassInfo& class_info)
{
    reflect::RangeConstraint<int> teeth_constraint{};
    teeth_constraint.min = gear_limits::min_teeth;
    teeth_constraint.max = gear_limits::max_teeth;
    teeth_constraint.step = 1;
    teeth_constraint.clamp = true;

    reflect::RangeConstraint<float> tooth_depth_constraint{};
    tooth_depth_constraint.min = gear_limits::min_tooth_depth;
    tooth_depth_constraint.max = gear_limits::max_tooth_depth;
    tooth_depth_constraint.step = 0.01f;
    tooth_depth_constraint.clamp = true;

    reflect::RangeConstraint<float> inner_radius_constraint{};
    inner_radius_constraint.min = gear_limits::min_inner_radius;
    inner_radius_constraint.max = gear_limits::max_inner_radius;
    inner_radius_constraint.step = 0.01f;
    inner_radius_constraint.clamp = true;

    reflect::RangeConstraint<float> outer_radius_constraint{};
    outer_radius_constraint.min = gear_limits::min_outer_radius;
    outer_radius_constraint.max = gear_limits::max_outer_radius;
    outer_radius_constraint.step = 0.01f;
    outer_radius_constraint.clamp = true;

    reflect::RangeConstraint<float> width_constraint{};
    width_constraint.min = gear_limits::min_width;
    width_constraint.max = gear_limits::max_width;
    width_constraint.step = 0.01f;
    width_constraint.clamp = true;

    reflect::register_property<&Gear::inner_radius>(
      class_info,
      "inner_radius",
      "Inner Radius",
      reflect::PropertyFlags::None,
      inner_radius_constraint);
    reflect::register_property<&Gear::outer_radius>(
      class_info,
      "outer_radius",
      "Outer Radius",
      reflect::PropertyFlags::None,
      outer_radius_constraint);
    reflect::register_property<&Gear::width>(
      class_info,
      "width",
      "Width",
      reflect::PropertyFlags::None,
      width_constraint);
    reflect::register_property<&Gear::teeth>(
      class_info,
      "teeth",
      "Teeth",
      reflect::PropertyFlags::None,
      teeth_constraint);
    reflect::register_property<&Gear::tooth_depth>(
      class_info,
      "tooth_depth",
      "Tooth Depth",
      reflect::PropertyFlags::None,
      tooth_depth_constraint);
}

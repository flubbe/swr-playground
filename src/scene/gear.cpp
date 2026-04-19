/**
 * Software Rasterizer Playground.
 *
 * Create gear geometry.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

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
    GearGeometry gear_geom;

    float r0 = inner_radius;
    float r1 = outer_radius - tooth_depth / 2.f;
    float r2 = outer_radius + tooth_depth / 2.f;

    float da = 2.f * static_cast<float>(M_PI / teeth) / 4.f;

    std::vector<ml::vec4> vb;
    std::vector<ml::vec4> nb;
    std::vector<std::uint32_t> ib;

    /* draw front face */
    for(int i = 0; i <= teeth; ++i)
    {
        float angle = i * 2.f * static_cast<float>(M_PI) / static_cast<float>(teeth);
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
    da = 2.f * static_cast<float>(M_PI) / static_cast<float>(teeth) / 4.f;
    for(int i = 0; i < teeth; ++i)
    {
        float angle = i * 2.f * static_cast<float>(M_PI) / static_cast<float>(teeth);

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
        float angle = i * 2.f * static_cast<float>(M_PI) / static_cast<float>(teeth);
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
    da = 2.f * static_cast<float>(M_PI) / static_cast<float>(teeth) / 4.f;
    for(int i = 0; i < teeth; ++i)
    {
        float angle = i * 2.f * static_cast<float>(M_PI) / static_cast<float>(teeth);

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

    /* draw outward faces of teeth */
    for(int i = 0; i < teeth; ++i)
    {
        float angle = i * 2.f * static_cast<float>(M_PI) / static_cast<float>(teeth);

        vb.emplace_back(r1 * std::cos(angle), r1 * std::sin(angle), width * 0.5f);
        vb.emplace_back(r1 * std::cos(angle), r1 * std::sin(angle), -width * 0.5f);

        ml::vec4 uv{
          r2 * std::sin(angle + da) - r1 * std::sin(angle),
          -r2 * std::cos(angle + da) + r1 * std::cos(angle),
          0, 0};
        nb.emplace_back(uv.normalized());
        nb.emplace_back(uv.normalized());

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

        vb.emplace_back(r2 * std::cos(angle + da), r2 * std::sin(angle + da), width * 0.5f);
        vb.emplace_back(r2 * std::cos(angle + da), r2 * std::sin(angle + da), -width * 0.5f);

        nb.emplace_back(std::cos(angle), std::sin(angle), 0, 0);
        nb.emplace_back(std::cos(angle), std::sin(angle), 0, 0);

        auto cur_idx = vb.size() - 1;
        ib.emplace_back(cur_idx - 2);
        ib.emplace_back(cur_idx - 1);
        ib.emplace_back(cur_idx - 3);

        ib.emplace_back(cur_idx - 2);
        ib.emplace_back(cur_idx);
        ib.emplace_back(cur_idx - 1);

        vb.emplace_back(r2 * std::cos(angle + 2 * da), r2 * std::sin(angle + 2 * da), width * 0.5f);
        vb.emplace_back(r2 * std::cos(angle + 2 * da), r2 * std::sin(angle + 2 * da), -width * 0.5f);

        uv = ml::vec4{
          r1 * std::sin(angle + 3 * da) - r2 * std::sin(angle + 2 * da),
          -r1 * std::cos(angle + 3 * da) + r2 * std::cos(angle + 2 * da),
          0, 0};
        nb.emplace_back(uv.normalized());
        nb.emplace_back(uv.normalized());

        cur_idx = vb.size() - 1;
        ib.emplace_back(cur_idx - 3);
        ib.emplace_back(cur_idx - 2);
        ib.emplace_back(cur_idx - 1);

        ib.emplace_back(cur_idx - 2);
        ib.emplace_back(cur_idx);
        ib.emplace_back(cur_idx - 1);

        vb.emplace_back(r1 * std::cos(angle + 3 * da), r1 * std::sin(angle + 3 * da), width * 0.5f);
        vb.emplace_back(r1 * std::cos(angle + 3 * da), r1 * std::sin(angle + 3 * da), -width * 0.5f);

        nb.emplace_back(std::cos(angle), std::sin(angle), 0, 0);
        nb.emplace_back(std::cos(angle), std::sin(angle), 0, 0);

        cur_idx = vb.size() - 1;
        ib.emplace_back(cur_idx - 2);
        ib.emplace_back(cur_idx - 1);
        ib.emplace_back(cur_idx - 3);

        ib.emplace_back(cur_idx - 2);
        ib.emplace_back(cur_idx);
        ib.emplace_back(cur_idx - 1);
    }

    vb.emplace_back(r1 * std::cos(0.f), r1 * std::sin(0.f), width * 0.5f);
    vb.emplace_back(r1 * std::cos(0.f), r1 * std::sin(0.f), -width * 0.5f);

    nb.emplace_back(std::cos(0.f), std::sin(0.f), 0, 0);
    nb.emplace_back(std::cos(0.f), std::sin(0.f), 0, 0);

    auto cur_idx = vb.size() - 1;
    ib.emplace_back(cur_idx - 2);
    ib.emplace_back(cur_idx - 1);
    ib.emplace_back(cur_idx - 3);

    ib.emplace_back(cur_idx - 2);
    ib.emplace_back(cur_idx);
    ib.emplace_back(cur_idx - 1);

    /* create outside of the gear. */
    gear_geom.outer_vertices = std::move(vb);
    gear_geom.outer_normals = std::move(nb);
    gear_geom.outer_indices = std::move(ib);

    /* clear buffers for the inner cylinder. */
    vb.clear();
    nb.clear();
    ib.clear();

    /* draw inside radius cylinder */
    for(int i = 0; i <= teeth; i++)
    {
        float angle = i * 2.f * static_cast<float>(M_PI) / static_cast<float>(teeth);
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

DEFINE_CLASS(Gear);

Gear::Gear(
  const GearParameters& params)
: Object{
    std::vector{
      RenderData{
        .mesh_handle = params.inner.mesh_handle,
        .material_handle = params.inner.material_handle},
      RenderData{
        .mesh_handle = params.outer.mesh_handle,
        .material_handle = params.outer.material_handle}}}
{
}

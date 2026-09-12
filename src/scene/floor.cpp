/**
 * Software Rasterizer Playground.
 *
 * Create procedural floor geometry.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include "reflection/builtin_properties.h"
#include "asset_resolver.h"
#include "floor.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

DEFINE_REFLECTION(Floor);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void Floor::post_load()
{
    mark_mesh_dirty();
}

MeshData Floor::generate_mesh() const
{
    const ml::vec4 up_normal{0.f, 1.f, 0.f, 0.f};

    return MeshData{
      .primitive_type = PrimitiveType::Triangles,
      .indices = {0, 2, 1, 0, 3, 2},
      .vertices = {
        {-half_extent, 0.f, -half_extent, 1.f},
        {half_extent, 0.f, -half_extent, 1.f},
        {half_extent, 0.f, half_extent, 1.f},
        {-half_extent, 0.f, half_extent, 1.f},
      },
      .normals = {up_normal, up_normal, up_normal, up_normal},
      .texcoords = {
        {0.f, uv_repeat, 0.f, 0.f},
        {uv_repeat, uv_repeat, 0.f, 0.f},
        {uv_repeat, 0.f, 0.f, 0.f},
        {0.f, 0.f, 0.f, 0.f},
      },
    };
}

void Floor::register_properties(
  reflect::ClassInfo& class_info)
{
    reflect::RangeConstraint<float> extent_constraint{};
    extent_constraint.min = 0.01f;
    extent_constraint.max = 1000.f;
    extent_constraint.step = 0.01f;
    extent_constraint.clamp = true;

    reflect::RangeConstraint<float> uv_repeat_constraint{};
    uv_repeat_constraint.min = 0.01f;
    uv_repeat_constraint.max = 1000.f;
    uv_repeat_constraint.step = 0.01f;
    uv_repeat_constraint.clamp = true;

    class_info.register_property<&Floor::half_extent>(
      "half_extent",
      "Half Extent",
      reflect::PropertyFlags::None,
      extent_constraint);
    class_info.register_property<&Floor::uv_repeat>(
      "uv_repeat",
      "UV Repeat",
      reflect::PropertyFlags::None,
      uv_repeat_constraint);
}

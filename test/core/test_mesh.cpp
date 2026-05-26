#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "mesh.h"
#include "mesh_simplifier.h"

TEST(MeshTests, MeshSimplifierCollapsesClosedMeshEdges)
{
    constexpr float scale = 0.01f;

    const MeshData mesh{
      .primitive_type = PrimitiveType::Triangles,
      .indices =
        {
          0,
          4,
          3,
          0,
          3,
          5,
          0,
          5,
          2,
          0,
          2,
          4,
          1,
          3,
          4,
          1,
          5,
          3,
          1,
          2,
          5,
          1,
          4,
          2,
        },
      .vertices =
        {
          {0.f, scale, 0.f, 1.f},
          {0.f, -scale, 0.f, 1.f},
          {-scale, 0.f, 0.f, 1.f},
          {scale, 0.f, 0.f, 1.f},
          {0.f, 0.f, scale, 1.f},
          {0.f, 0.f, -scale, 1.f},
        },
      .normals =
        {
          {0.f, 1.f, 0.f, 0.f},
          {0.f, -1.f, 0.f, 0.f},
          {-1.f, 0.f, 0.f, 0.f},
          {1.f, 0.f, 0.f, 0.f},
          {0.f, 0.f, 1.f, 0.f},
          {0.f, 0.f, -1.f, 0.f},
        },
    };

    MeshSimplifier simplifier;
    const MeshData lod = simplifier.simplify(
      mesh,
      MeshSimplifySettings{.target_triangle_fraction = 0.5f});
    const MeshSimplifyStats& stats = simplifier.stats();

    EXPECT_EQ(lod.primitive_type, PrimitiveType::Triangles);
    EXPECT_GT(stats.accepted_collapses, 0U);
    EXPECT_LT(lod.indices.size(), mesh.indices.size());
    EXPECT_EQ(lod.indices.size() % 3, 0U);
    EXPECT_EQ(lod.vertices.size(), lod.normals.size());

    for(const std::uint32_t index: lod.indices)
    {
        EXPECT_LT(index, lod.vertices.size());
    }

    for(std::size_t i = 0; i + 2 < lod.indices.size(); i += 3)
    {
        EXPECT_NE(lod.indices[i + 0], lod.indices[i + 1]);
        EXPECT_NE(lod.indices[i + 0], lod.indices[i + 2]);
        EXPECT_NE(lod.indices[i + 1], lod.indices[i + 2]);
    }
}

TEST(MeshTests, MeshSimplifierLeavesLinesUnchanged)
{
    const MeshData mesh{
      .primitive_type = PrimitiveType::Lines,
      .indices = {0, 1, 2, 3},
      .vertices =
        {
          {0.f, 0.f, 0.f, 1.f},
          {1.f, 0.f, 0.f, 1.f},
          {1.f, 1.f, 0.f, 1.f},
          {0.f, 1.f, 0.f, 1.f},
        },
      .normals =
        {
          {0.f, 1.f, 0.f, 0.f},
          {0.f, 1.f, 0.f, 0.f},
          {0.f, 1.f, 0.f, 0.f},
          {0.f, 1.f, 0.f, 0.f},
        },
    };

    MeshSimplifier simplifier;
    const MeshData lod = simplifier.simplify(
      mesh,
      MeshSimplifySettings{.target_triangle_fraction = 0.25f});

    EXPECT_EQ(lod.primitive_type, PrimitiveType::Lines);
    EXPECT_EQ(lod.indices, mesh.indices);
    EXPECT_EQ(lod.vertices.size(), mesh.vertices.size());
}

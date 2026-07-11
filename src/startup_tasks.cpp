/**
 * Software Rasterizer Playground.
 *
 * Startup task definitions.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <algorithm>
#include <array>
#include <limits>
#include <mutex>

#include "assets/static_mesh_importer.h"
#include "logging.h"
#include "mesh_lod.h"
#include "startup_tasks.h"

namespace
{

std::mutex startup_notice_mutex;

void add_startup_notice(
  PreparedStartupScene& scene,
  std::string notice)
{
    std::lock_guard lock{startup_notice_mutex};
    scene.notices.push_back(std::move(notice));
}

struct GearInit
{
    ml::vec4 color;
    float inner_radius;
    float outer_radius;
    float width;
    int teeth;
    float tooth_depth;
    ml::mat4x4 transform;
    ml::vec3 translation;
    float angular_speed;
    float phase_offset;
};

MeshBounds calculate_imported_mesh_bounds(
  const ImportedStaticMesh& imported_mesh)
{
    MeshBounds bounds;
    for(const auto& mesh: imported_mesh.meshes)
    {
        expand_bounds(
          bounds,
          calculate_mesh_bounds(mesh.mesh_data));
    }

    return bounds;
}

float calculate_max_half_extent(const MeshBounds& bounds)
{
    if(!bounds.valid)
    {
        return 0.f;
    }

    const ml::vec3 extents = bounds.max - bounds.min;
    return 0.5f * std::max({extents.x, extents.y, extents.z});
}

ml::vec3 calculate_center(const MeshBounds& bounds)
{
    return (bounds.min + bounds.max) * 0.5f;
}

ml::mat4x4 make_static_mesh_fit_transform(
  const MeshBounds& bounds,
  float target_half_extent)
{
    const float max_half_extent = calculate_max_half_extent(bounds);
    if(max_half_extent <= std::numeric_limits<float>::epsilon())
    {
        return ml::mat4x4::identity();
    }

    const ml::vec3 center = calculate_center(bounds);
    const float scale = target_half_extent / max_half_extent;

    return ml::matrices::scaling(scale)
           * ml::matrices::translation(-center);
}

MeshData make_floor_mesh(
  float half_extent,
  float uv_repeat)
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
      .normals = {
        up_normal,
        up_normal,
        up_normal,
        up_normal,
      },
      .texcoords = {
        {0.f, uv_repeat, 0.f, 0.f},
        {uv_repeat, uv_repeat, 0.f, 0.f},
        {uv_repeat, 0.f, 0.f, 0.f},
        {0.f, 0.f, 0.f, 0.f},
      },
    };
}

std::optional<PreparedFloorData> try_prepare_floor_data()
{
    const std::filesystem::path diffuse_path{
      "assets/textures/tiles/tiles_0080_color_1k.png"};
    const std::filesystem::path normal_path{
      "assets/textures/tiles/tiles_0080_normal_opengl_1k.png"};

    if(!std::filesystem::exists(diffuse_path)
       || !std::filesystem::exists(normal_path))
    {
        return std::nullopt;
    }

    constexpr float floor_half_extent = 28.f;
    constexpr float uv_repeat = 1.f;
    return PreparedFloorData{
      .mesh = make_floor_mesh(floor_half_extent, uv_repeat),
      .diffuse_texture = assets::load_texture_rgba8(diffuse_path),
      .normal_texture = assets::load_normal_map_rgba8(
        normal_path,
        assets::NormalMapConvention::DirectX),
    };
}

std::vector<PreparedStaticMeshSection> build_static_mesh_sections(
  ImportedStaticMesh imported_mesh)
{
    const StaticMeshLodBuildSettings lod_settings{
      .preserve_boundaries = false,
      .recompute_normals = true,
    };

    StaticMeshLodBuilder lod_builder;
    std::vector<PreparedStaticMeshSection> sections;
    sections.reserve(imported_mesh.meshes.size());

    for(auto& mesh: imported_mesh.meshes)
    {
        const StaticMeshLodBuildResult lod_build_result =
          lod_builder.build(
            mesh.mesh_data,
            lod_settings);

        PreparedStaticMeshSection section{
          .diffuse_color = mesh.diffuse_color,
        };
        section.lods.reserve(lod_build_result.lod_meshes.size());

        for(const auto& lod_mesh: lod_build_result.lod_meshes)
        {
            section.lods.push_back(
              PreparedStaticMeshSectionLod{
                .mesh = lod_mesh.mesh,
                .min_screen_height = lod_mesh.min_screen_height,
                .bounds = calculate_mesh_bounds(lod_mesh.mesh),
              });
        }

        sections.push_back(std::move(section));
    }

    return sections;
}

std::optional<PreparedStaticMeshAsset> try_prepare_sample_mesh(
  const std::filesystem::path& static_mesh_path)
{
    if(!std::filesystem::exists(static_mesh_path))
    {
        return std::nullopt;
    }

    constexpr float sample_half_extent = 2.f;

    ImportedStaticMesh imported_mesh = import_static_mesh(static_mesh_path);
    const MeshBounds mesh_bounds =
      calculate_imported_mesh_bounds(imported_mesh);

    auto sections = build_static_mesh_sections(std::move(imported_mesh));
    std::erase_if(
      sections,
      [](const PreparedStaticMeshSection& section)
      {
          return section.lods.empty();
      });

    if(sections.empty())
    {
        return std::nullopt;
    }

    return PreparedStaticMeshAsset{
      .name = static_mesh_path.filename().string(),
      .fit_transform = make_static_mesh_fit_transform(
        mesh_bounds,
        sample_half_extent),
      .sections = std::move(sections),
    };
}

}    // namespace

namespace startup_tasks
{

using task_system::TaskCancelledError;
using task_system::TaskExecutionContext;
using task_system::TaskSpec;

[[nodiscard]]
const logging::Logger& get_startup_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger startup_logger{"Startup"};
    return startup_logger;
}

// Create a task that generates procedural gears
[[nodiscard]]
TaskSpec make_gear_task(PreparedStartupScene& scene)
{
    return TaskSpec{
      .name = "Generating procedural scene data...",
      .weight = 1.f,
      .run = [&scene](TaskExecutionContext& context)
      {
          context.update("Generating procedural scene data...", 0.f);
          get_startup_logger().logf("generating procedural scene data");

          const std::array<GearInit, 3> gears = {{
            {
              .color = {1, 0, 0, 1},
              .inner_radius = 1.0f,
              .outer_radius = 4.0f,
              .width = 1.0f,
              .teeth = 20,
              .tooth_depth = 0.7f,
              .transform = ml::matrices::translation(-3.f, -2.f, 0.f),
              .translation = {-3.f, -2.f, 0.f},
              .angular_speed = 1.f,
              .phase_offset = 0.f,
            },
            {
              .color = {0, 1, 0, 1},
              .inner_radius = 0.5f,
              .outer_radius = 2.0f,
              .width = 2.0f,
              .teeth = 10,
              .tooth_depth = 0.7f,
              .transform = ml::matrices::translation(3.1f, -2.f, 0.f),
              .translation = {3.1f, -2.f, 0.f},
              .angular_speed = -2.f,
              .phase_offset = -9.f,
            },
            {
              .color = {0, 0, 1, 1},
              .inner_radius = 1.3f,
              .outer_radius = 2.0f,
              .width = 0.5f,
              .teeth = 10,
              .tooth_depth = 0.7f,
              .transform = ml::matrices::translation(-3.1f, 4.2f, 0.f),
              .translation = {-3.1f, 4.2f, 0.f},
              .angular_speed = -2.f,
              .phase_offset = -25.f,
            },
          }};

          scene.gears.reserve(gears.size());
          for(const GearInit& gear: gears)
          {
              scene.gears.push_back(
                PreparedGearInstance{
                  .color = gear.color,
                  .inner_radius = gear.inner_radius,
                  .outer_radius = gear.outer_radius,
                  .width = gear.width,
                  .teeth = gear.teeth,
                  .tooth_depth = gear.tooth_depth,
                  .geometry = make_gear(
                    gear.inner_radius,
                    gear.outer_radius,
                    gear.width,
                    gear.teeth,
                    gear.tooth_depth),
                  .transform = gear.transform,
                  .translation = gear.translation,
                  .angular_speed = gear.angular_speed,
                  .phase_offset = gear.phase_offset,
                });
          }

          context.update("Generated procedural scene data.", 1.f);
      },
    };
}

// Create a task that loads floor textures and mesh
[[nodiscard]]
TaskSpec make_floor_task(PreparedStartupScene& scene)
{
    return TaskSpec{
      .name = "Loading floor textures...",
      .weight = 2.f,
      .run = [&scene](TaskExecutionContext& context)
      {
          context.update("Loading floor textures...", 0.f);
          get_startup_logger().logf("loading floor textures");
          scene.floor = try_prepare_floor_data();
          if(!scene.floor.has_value())
          {
              add_startup_notice(
                scene,
                "floor textures were not found");
          }
          context.update("Loaded floor textures.", 1.f);
      },
    };
}

// Create a task that imports the sample mesh
[[nodiscard]]
TaskSpec make_sample_mesh_task(PreparedStartupScene& scene)
{
    return TaskSpec{
      .name = "Importing sample mesh...",
      .weight = 3.f,
      .run = [&scene](TaskExecutionContext& context)
      {
          const std::filesystem::path sample_mesh_path{"assets/models/bunny.obj"};

          context.update(
            std::format(
              "Importing {}...",
              sample_mesh_path.string()),
            0.f);
          get_startup_logger().logf(
            "importing sample mesh '{}'",
            sample_mesh_path.generic_string());
          scene.sample_mesh = try_prepare_sample_mesh(sample_mesh_path);
          if(!scene.sample_mesh.has_value())
          {
              add_startup_notice(
                scene,
                std::string{"sample static mesh was not found or had no renderable data: "}
                  + sample_mesh_path.generic_string());
          }
          context.update("Startup data prepared.", 1.f);
      },
    };
}

std::vector<TaskSpec> create_startup_tasks(PreparedStartupScene& scene)
{
    return {
      make_gear_task(scene),
      make_floor_task(scene),
      make_sample_mesh_task(scene),
    };
}

}    // namespace startup_tasks

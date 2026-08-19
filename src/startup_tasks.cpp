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
#include <format>
#include <limits>
#include <mutex>

#include "assets/static_mesh_importer.h"
#include "containers/string.h"
#include "meshes/lod.h"
#include "serialization/file.h"
#include "serialization/hash.h"
#include "colors.h"
#include "logging.h"
#include "startup_tasks.h"

namespace
{

[[nodiscard]]
const logging::Logger& get_logger()
{
    // Create on first use so it binds after logging initialization.
    static const logging::Logger logger{"Startup"};
    return logger;
}

void add_startup_notice(
  StagedStartupScene& scene,
  std::string_view notice)
{
    std::lock_guard lock{scene.notices_mutex};
    scene.notices.emplace_back(notice.data(), notice.size());
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
  ImportedStaticMesh& imported_mesh)
{
    MeshBounds bounds;

    for(auto& mesh: imported_mesh.meshes)
    {
        expand_bounds(bounds, mesh.bounds);
    }

    bounds.center = (bounds.min + bounds.max) * 0.5f;
    bounds.radius = (bounds.center - bounds.max).length();

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

std::optional<StagedFloorData> try_prepare_floor_data()
{
    constexpr float floor_half_extent = 28.f;
    constexpr float uv_repeat = 1.f;
    return StagedFloorData{
      .mesh = make_floor_mesh(floor_half_extent, uv_repeat),
    };
}

// FIXME duplicated in mesh_manager.cpp, should likely be removed here.
swr::vector<StagedStaticMeshSection> build_static_mesh_sections(
  ImportedStaticMesh imported_mesh)
{
    const StaticMeshLodBuildSettings lod_settings{
      .preserve_boundaries = false,
      .recompute_normals = true,
    };

    StaticMeshLodBuilder lod_builder;
    swr::vector<StagedStaticMeshSection> sections;
    sections.reserve(imported_mesh.meshes.size());

    for(auto& mesh: imported_mesh.meshes)
    {
        const StaticMeshLodBuildResult lod_build_result =
          lod_builder.build(
            mesh.mesh_data,
            lod_settings);

        StagedStaticMeshSection section{
          .diffuse_color = mesh.diffuse_color,
          .lods = {}};
        section.lods.reserve(lod_build_result.lod_meshes.size());

        for(const auto& lod_mesh: lod_build_result.lod_meshes)
        {
            section.lods.push_back(
              StagedStaticMeshSectionLod{
                .mesh = lod_mesh.mesh,
                .bounds = calculate_mesh_bounds(lod_mesh.mesh),
              });
        }

        sections.push_back(std::move(section));
    }

    return sections;
}

std::uint64_t compute_mesh_cache_key(
  const std::filesystem::path& static_mesh_path)
{
    if(!std::filesystem::exists(static_mesh_path))
    {
        throw std::runtime_error{
          std::format(
            "Cannot compute hash for non-existing mesh '{}'.",
            static_mesh_path.string())};
    }

    serial::HashArchive hash_ar;
    serial::FileReadArchive file_ar{
      static_mesh_path};

    const std::size_t file_size = file_ar.size();

    constexpr std::size_t buffer_size = 64 * 1024;
    std::array<std::byte, buffer_size> buffer;

    while(file_ar.tell() < file_size)
    {
        const std::size_t remaining = file_size - file_ar.tell();
        const std::size_t read_size = std::min(buffer_size, remaining);
        std::span<std::byte> bytes{buffer.data(), read_size};

        file_ar.serialize(bytes);
        hash_ar.serialize(bytes);
    }

    return hash_ar.digest();
}

std::optional<StagedStaticMeshAsset> try_load_cached_mesh(
  const std::filesystem::path& path)
{
    StagedStaticMeshAsset mesh;

    try
    {
        serial::FileReadArchive ar{path};
        ar & mesh;
    }
    catch(...)
    {
        return std::nullopt;
    }

    return mesh;
}

std::optional<StagedStaticMeshAsset> try_prepare_sample_mesh(
  const std::filesystem::path& static_mesh_path)
{
    if(!std::filesystem::exists(static_mesh_path))
    {
        return std::nullopt;
    }

    const std::uint64_t cache_key = compute_mesh_cache_key(static_mesh_path);
    auto cache_path =
      std::filesystem::path{"cache"} / "meshes" / std::format("{:016x}.lodmesh", cache_key);

    if(std::filesystem::exists(cache_path))
    {
        if(auto cached = try_load_cached_mesh(cache_path))
        {
            get_logger().logf(
              "Using cache entry for '{}' (hash: {:016x}).",
              static_mesh_path.string(),
              cache_key);

            return cached;
        }
    }

    get_logger().logf(
      "No cache entry found for '{}' (hash: {:016x}).",
      static_mesh_path.string(),
      cache_key);

    constexpr float sample_half_extent = 2.f;

    ImportedStaticMesh imported_mesh = import_static_mesh(static_mesh_path);
    const MeshBounds mesh_bounds =
      calculate_imported_mesh_bounds(imported_mesh);

    // TODO Fix color space. This works for some models.
    for(auto& mesh: imported_mesh.meshes)
    {
        mesh.diffuse_color = colors::linear_to_srgb(mesh.diffuse_color);
    }

    auto sections = build_static_mesh_sections(std::move(imported_mesh));
    std::erase_if(
      sections,
      [](const StagedStaticMeshSection& section)
      {
          return section.lods.empty();
      });

    if(sections.empty())
    {
        return std::nullopt;
    }

    auto mesh_asset = StagedStaticMeshAsset{
      .path = assets::AssetPath{static_mesh_path},
      .fit_transform = make_static_mesh_fit_transform(
        mesh_bounds,
        sample_half_extent),
      .sections = std::move(sections),
    };

    // Write the processed asset.
    std::error_code ec;
    std::filesystem::create_directories(cache_path.parent_path(), ec);

    if(ec)
    {
        get_logger().errorf(
          "Cannot write cached asset. Failed to create directory structure '{}': {}",
          cache_path.parent_path().string(),
          ec.message());
    }
    else
    {
        serial::FileWriteArchive ar{cache_path};
        ar & mesh_asset;
    }

    return mesh_asset;
}

}    // namespace

namespace startup_tasks
{

using task_system::TaskCancelledError;
using task_system::TaskExecutionContext;
using task_system::TaskSpec;

// Create a task that generates procedural gears
[[nodiscard]]
TaskSpec make_gear_task(StagedStartupScene& scene)
{
    return TaskSpec{
      .name = "Generating procedural scene data...",
      .weight = 1.f,
      .run = [&scene](TaskExecutionContext& context)
      {
          context.update("Generating procedural scene data...", 0.f);
          get_logger().logf("generating procedural scene data");

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
                StagedGearInstance{
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

// Create a task that generates the floor mesh.
[[nodiscard]]
TaskSpec make_floor_task(StagedStartupScene& scene)
{
    return TaskSpec{
      .name = "Generating floor mesh...",
      .weight = 2.f,
      .run = [&scene](TaskExecutionContext& context)
      {
          context.update("Generating floor mesh...", 0.f);
          get_logger().logf("generating floor mesh");
          scene.floor = try_prepare_floor_data();
          if(!scene.floor.has_value())
          {
              add_startup_notice(
                scene,
                "floor mesh could not be generated");
          }
          context.update("Generated floor mesh.", 1.f);
      },
    };
}

// Create a task that imports the sample mesh
[[nodiscard]]
TaskSpec make_sample_mesh_task(StagedStartupScene& scene)
{
    const std::array<std::filesystem::path, 3> sample_mesh_paths = {
      "assets/models/bunny.obj",
      "assets/models/cars/COP.obj",
      "assets/models/bunny.obj"};

    return TaskSpec{
      .name = sample_mesh_paths.size() == 1
                ? "Importing sample mesh..."
                : "Importing sample meshes...",
      .weight = 3.f,
      .run = [&scene, sample_mesh_paths](TaskExecutionContext& context)
      {
          for(const auto& path: sample_mesh_paths)
          {
              context.update(
                std::format(
                  "Importing {}...",
                  path.string()),
                0.f);
              get_logger().logf(
                "importing sample mesh '{}'",
                path.string());
              auto sample_mesh = try_prepare_sample_mesh(path);
              if(!sample_mesh.has_value())
              {
                  add_startup_notice(
                    scene,
                    std::format(
                      "sample static mesh was not found or had no renderable data: {}",
                      path.string()));
              }
              else
              {
                  scene.sample_meshes.emplace_back(
                    std::move(sample_mesh.value()));
              }
          }

          if(sample_mesh_paths.size() == 1)
          {
              context.update("Mesh loaded.", 1.f);
          }
          else
          {
              context.update("Meshes loaded.", 1.f);
          }
      },
    };
}

swr::vector<TaskSpec> create_startup_tasks(StagedStartupScene& scene)
{
    return {
      make_gear_task(scene),
      make_floor_task(scene),
      make_sample_mesh_task(scene),
    };
}

}    // namespace startup_tasks

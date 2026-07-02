#include <gtest/gtest.h>

#include "reflection/class_registry.h"
#include "scene/scene.h"
#include "scene/static_mesh.h"

namespace
{

void ensure_scene_reflection_ready()
{
    static bool initialized = false;
    if(initialized)
    {
        return;
    }

    reflect::ReflectionSystem::allow_auto_registration(false);
    reflect::ReflectionSystem::process_pending_registrations();
    initialized = true;
}

}    // namespace

TEST(SceneTests, AddObjectSynchronizesObjectIndex)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Camera* camera = scene.add_object<Camera>();
    ASSERT_NE(camera, nullptr);

    const ObjectId id = camera->get_object_id();
    EXPECT_EQ(scene.find_object(id), camera);
    ASSERT_EQ(scene.get_objects_by_id().count(id), 1U);
    EXPECT_EQ(scene.get_objects_by_id().at(id), camera);
}

TEST(SceneTests, TypedFindObjectFiltersByRequestedType)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Camera* camera = scene.add_object<Camera>();
    StaticMesh* mesh = scene.add_object<StaticMesh>();
    ASSERT_NE(camera, nullptr);
    ASSERT_NE(mesh, nullptr);

    EXPECT_EQ(scene.find_object<Camera>(camera->get_object_id()), camera);
    EXPECT_EQ(scene.find_object<StaticMesh>(mesh->get_object_id()), mesh);
    EXPECT_EQ(scene.find_object<StaticMesh>(camera->get_object_id()), nullptr);
    EXPECT_EQ(scene.find_camera(camera->get_object_id()), camera);
    EXPECT_EQ(scene.find_camera(mesh->get_object_id()), nullptr);
}

TEST(SceneTests, GetCamerasReturnsOnlyCameraObjects)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Camera* first_camera = scene.add_object<Camera>();
    [[maybe_unused]] StaticMesh* mesh = scene.add_object<StaticMesh>();
    Camera* second_camera = scene.add_object<Camera>();
    ASSERT_NE(first_camera, nullptr);
    ASSERT_NE(second_camera, nullptr);

    const std::vector<Camera*> cameras = scene.get_cameras();
    ASSERT_EQ(cameras.size(), 2U);
    EXPECT_EQ(cameras[0], first_camera);
    EXPECT_EQ(cameras[1], second_camera);
}

TEST(SceneTests, ClearRemovesObjectIndexAndSpinAnimations)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Camera* camera = scene.add_object<Camera>();
    ASSERT_NE(camera, nullptr);
    const ObjectId id = camera->get_object_id();

    scene.set_spin_animation(
      id,
      SpinAnimation{
        .translation = {1.f, 2.f, 3.f},
        .angular_speed = 2.f,
        .phase_offset = 5.f});
    ASSERT_EQ(scene.get_spin_animations().count(id), 1U);
    ASSERT_EQ(scene.get_objects_by_id().count(id), 1U);

    scene.clear();

    EXPECT_TRUE(scene.get_objects().empty());
    EXPECT_TRUE(scene.get_objects_by_id().empty());
    EXPECT_TRUE(scene.get_spin_animations().empty());
    EXPECT_EQ(scene.find_object(id), nullptr);
}

TEST(SceneTests, AddStaticMeshStoresMeshSections)
{
    ensure_scene_reflection_ready();

    Scene scene;
    StaticMesh* mesh = scene.add_object<StaticMesh>(
      std::vector{
        MeshSection{
          .mesh_handle = {.value = 12},
          .material_handle = {.value = 34}}});
    ASSERT_NE(mesh, nullptr);

    EXPECT_TRUE(mesh->is_a<StaticMesh>());
    ASSERT_EQ(mesh->get_mesh_sections().size(), 1U);
    EXPECT_EQ(mesh->get_mesh_sections()[0].mesh_handle, MeshHandle{.value = 12U});
    EXPECT_EQ(mesh->get_mesh_sections()[0].material_handle, MaterialHandle{.value = 34U});
    EXPECT_EQ(scene.find_object(mesh->get_object_id()), mesh);
}

TEST(SceneTests, StaticMeshSelectsLodFromScreenHeight)
{
    ensure_scene_reflection_ready();

    StaticMesh mesh{
      std::vector{
        StaticMeshLod{
          .mesh_sections =
            {
              MeshSection{
                .mesh_handle = {.value = 10},
                .material_handle = {.value = 20},
              },
            },
          .min_screen_height = 0.4f,
          .bounds = {}},
        StaticMeshLod{
          .mesh_sections =
            {
              MeshSection{
                .mesh_handle = {.value = 11},
                .material_handle = {.value = 21},
              },
            },
          .min_screen_height = 0.15f,
          .bounds = {}},
        StaticMeshLod{
          .mesh_sections =
            {
              MeshSection{
                .mesh_handle = {.value = 12},
                .material_handle = {.value = 22},
              },
            },
          .min_screen_height = 0.f,
          .bounds = {}},
      }};

    EXPECT_EQ(mesh.get_lod_count(), 3U);
    EXPECT_EQ(mesh.select_lod(0.5f), 0U);
    EXPECT_EQ(mesh.select_lod(0.2f), 1U);
    EXPECT_EQ(mesh.select_lod(0.05f), 2U);
}

TEST(SceneTests, StaticMeshStoresCachedBounds)
{
    ensure_scene_reflection_ready();

    const MeshBounds bounds{
      .min = {-1.f, -2.f, -3.f},
      .max = {1.f, 2.f, 3.f},
      .valid = true,
    };

    StaticMesh mesh{
      std::vector{
        MeshSection{
          .mesh_handle = {.value = 10},
          .material_handle = {.value = 20},
        },
      },
      bounds};

    EXPECT_TRUE(mesh.get_bounds().valid);
    EXPECT_EQ(mesh.get_bounds().min.x, -1.f);
    EXPECT_EQ(mesh.get_bounds().max.z, 3.f);

    mesh.clear_mesh_sections();

    EXPECT_FALSE(mesh.get_bounds().valid);
}

TEST(SceneTests, ForEachObjectVisitsRequestedType)
{
    ensure_scene_reflection_ready();

    Scene scene;
    [[maybe_unused]] Camera* camera = scene.add_object<Camera>();
    StaticMesh* mesh = scene.add_object<StaticMesh>(
      std::vector{
        MeshSection{
          .mesh_handle = {.value = 56},
          .material_handle = {.value = 78}}});
    ASSERT_NE(mesh, nullptr);

    int mutable_visit_count = 0;
    scene.for_each_object<StaticMesh>(
      [&mutable_visit_count, mesh](StaticMesh& object)
      {
          ++mutable_visit_count;
          EXPECT_EQ(&object, mesh);
          object.set_name("Visited Static Mesh");
      });

    EXPECT_EQ(mutable_visit_count, 1);
    EXPECT_EQ(mesh->get_name(), "Visited Static Mesh");

    const Scene& const_scene = scene;
    int const_visit_count = 0;
    const_scene.for_each_object<StaticMesh>(
      [&const_visit_count, mesh](const StaticMesh& object)
      {
          ++const_visit_count;
          EXPECT_EQ(object.get_object_id(), mesh->get_object_id());
      });

    EXPECT_EQ(const_visit_count, 1);
}

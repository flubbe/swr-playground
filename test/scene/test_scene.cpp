#include <gtest/gtest.h>

#include "reflection/class_registry.h"
#include "serialization/json/scene_loader.h"
#include "scene/scene.h"
#include "scene/static_mesh.h"

#include "../utils.h"

namespace fs = std::filesystem;

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

ResolvableMaterial make_test_material()
{
    return ResolvableMaterial{
      "Test",
      nullptr};
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

TEST(SceneTests, ObjectOfFiltersCameras)
{
    ensure_scene_reflection_ready();

    Scene scene;
    Camera* first_camera = scene.add_object<Camera>();
    [[maybe_unused]] StaticMesh* mesh = scene.add_object<StaticMesh>();
    Camera* second_camera = scene.add_object<Camera>();
    ASSERT_NE(first_camera, nullptr);
    ASSERT_NE(second_camera, nullptr);

    swr::vector<Camera*> cameras;
    for(auto& camera: scene.objects_of<Camera>())
    {
        static_assert(std::is_same_v<decltype(camera), Camera&>);
        cameras.emplace_back(&camera);
    }

    ASSERT_EQ(cameras.size(), 2U);
    EXPECT_EQ(cameras[0], first_camera);
    EXPECT_EQ(cameras[1], second_camera);

    swr::vector<const Camera*> const_cameras;
    for(auto& camera: std::as_const(scene).objects_of<Camera>())
    {
        static_assert(std::is_same_v<decltype(camera), const Camera&>);
        const_cameras.emplace_back(&camera);
    }

    ASSERT_EQ(const_cameras.size(), 2U);
    EXPECT_EQ(const_cameras[0], first_camera);
    EXPECT_EQ(const_cameras[1], second_camera);
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
      "<mesh>",
      swr::vector{
        MeshSection{
          .mesh_handle = {.value = 12},
          .material = make_test_material(),
          .triangle_count = 4}},
      MeshBounds{});
    ASSERT_NE(mesh, nullptr);

    EXPECT_TRUE(mesh->is_a<StaticMesh>());

    const auto& mesh_lods = mesh->get_lods();
    ASSERT_EQ(mesh_lods.size(), 1u);

    const auto& mesh_sections = mesh_lods[0].mesh_sections;
    ASSERT_EQ(mesh_sections.size(), 1u);

    EXPECT_EQ(mesh_sections[0].mesh_handle, MeshHandle{.value = 12U});

    EXPECT_EQ(scene.find_object(mesh->get_object_id()), mesh);
}

TEST(SceneTests, StaticMeshSelectsLodFromProjectedPixelArea)
{
    ensure_scene_reflection_ready();

    StaticMesh mesh;
    mesh.init(
      "<mesh>",
      swr::vector{
        StaticMeshLod{
          .mesh_sections =
            {
              MeshSection{
                .mesh_handle = {.value = 10},
                .material = make_test_material(),
                .triangle_count = 4},
            },
          .triangle_count = 100000,
          .bounds = {},
        },
        StaticMeshLod{
          .mesh_sections =
            {
              MeshSection{
                .mesh_handle = {.value = 11},
                .material = make_test_material(),
                .triangle_count = 4},
            },
          .triangle_count = 10000,
          .bounds = {},
        },
        StaticMeshLod{
          .mesh_sections =
            {
              MeshSection{
                .mesh_handle = {.value = 12},
                .material = make_test_material(),
                .triangle_count = 4},
            },
          .triangle_count = 1000,
          .bounds = {},
        },
      });

    EXPECT_EQ(mesh.get_lod_count(), 3U);

    constexpr float target = 2.0f;

    // 200000 / 100000 = 2 px/triangle
    EXPECT_EQ(mesh.select_lod(200000.0f, target), 0U);

    // 20000 / 100000 = 0.2  (reject LOD0)
    // 20000 / 10000  = 2.0  (accept LOD1)
    EXPECT_EQ(mesh.select_lod(20000.0f, target), 1U);

    // 2000 / 100000 = 0.02  (reject LOD0)
    // 2000 / 10000  = 0.2   (reject LOD1)
    // 2000 / 1000   = 2.0   (accept LOD2)
    EXPECT_EQ(mesh.select_lod(2000.0f, target), 2U);
}

TEST(SceneTests, StaticMeshStoresCachedBounds)
{
    ensure_scene_reflection_ready();

    const MeshBounds bounds{
      .min = {-1.f, -2.f, -3.f},
      .max = {1.f, 2.f, 3.f},
      .valid = true,
    };

    StaticMesh mesh;
    mesh.init(
      "<mesh>",
      swr::vector{
        MeshSection{
          .mesh_handle = {.value = 10},
          .material = make_test_material(),
          .triangle_count = 4},
      },
      bounds);

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
      "<mesh>",
      swr::vector{
        MeshSection{
          .mesh_handle = {.value = 56},
          .material = make_test_material(),
          .triangle_count = 4}},
      MeshBounds{});
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

TEST(SceneTests, EmptySaveLoad)
{
    ensure_scene_reflection_ready();

    {
        Scene scene;
        swr::string json;
        EXPECT_NO_THROW(json = scene.save(
                          0,      // indentation size (ignored)
                          true    // compacted
                          ));
        EXPECT_EQ(json,
                  "{\"time\":0,\"paused\":false,\"objects\":[]}");
    }

    {
        Scene scene;
        serial::json::JsonSceneLoader loader;

        EXPECT_NO_THROW(loader.load(scene, "{}"));

        std::size_t object_count{0};
        scene.for_each_object<Object>(
          [&]([[maybe_unused]] const Object& obj)
          {
              ++object_count;
          });

        EXPECT_EQ(object_count, 0);
    }
}

TEST(SceneTests, SaveLoad)
{
    ensure_scene_reflection_ready();

    const std::string_view expected =
      "{"
      "\"time\":0,"
      "\"paused\":false,"
      "\"objects\":["
      "{"
      "\"class\":\"Scene.StaticMesh\","
      "\"object_id\":1,"
      "\"name\":\"StaticMesh_1\","
      "\"transform\":[[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]],"
      "\"visible\":true,"
      "\"path\":\"\","
      "\"casts_shadows\":true,"
      "\"receives_shadows\":false"
      "}"
      "]}";

    {
        Scene scene;
        auto* mesh = scene.add_object<StaticMesh>();
        ASSERT_NE(mesh, nullptr);

        mesh->casts_shadows = true;
        mesh->receives_shadows = false;

        swr::string json;
        EXPECT_NO_THROW(json = scene.save(
                          0,      // indentation size (ignored)
                          true    // compacted
                          ));
        EXPECT_EQ(json, expected);
    }

    {
        Scene scene;
        serial::json::JsonSceneLoader loader;

        EXPECT_NO_THROW(loader.load(scene, expected));

        std::size_t object_count{0};
        scene.for_each_object<Object>(
          [&]([[maybe_unused]] const Object& obj)
          {
              ++object_count;
          });
        EXPECT_EQ(object_count, 1);

        std::size_t mesh_count{0};
        scene.for_each_object<StaticMesh>(
          [&]([[maybe_unused]] const StaticMesh& mesh)
          {
              ++mesh_count;
          });
        EXPECT_EQ(mesh_count, 1);
    }
}

add_executable(core_tests
    test_allocators.cpp
    test_hash.cpp
    test_logging.cpp
    test_file_manager.cpp
    test_materials.cpp
    test_mesh.cpp
    test_serialization.cpp
    test_task_system.cpp
    test_queue.cpp
    test_utils.cpp
    test_viewport.cpp
    test_camera_reflection.cpp
    ${PROJECT_SOURCE_DIR}/src/assets/material.cpp
    ${PROJECT_SOURCE_DIR}/src/logging.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/allocators/arena.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/allocators/bump.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/allocators/malloc.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/manager.cpp
    ${PROJECT_SOURCE_DIR}/src/meshes/mesh.cpp
    ${PROJECT_SOURCE_DIR}/src/meshes/simplifier.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/builtin_properties.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/class_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/properties.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/object.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/camera.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/light.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/directional_light.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/spotlight.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/scene.cpp
    ${PROJECT_SOURCE_DIR}/src/serialization/file.cpp
    ${PROJECT_SOURCE_DIR}/src/serialization/json/json_property_writer.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/dag.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/scheduler.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/state.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/task_system.cpp
    ${PROJECT_SOURCE_DIR}/src/file_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/viewport.cpp
)

target_include_directories(core_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${concurrency_utils_src_SOURCE_DIR}/include
)

target_compile_definitions(core_tests PRIVATE
    ASSETS_SOURCE_DIR="${CMAKE_SOURCE_DIR}/assets"
)

target_link_libraries(core_tests PRIVATE
    swr_playground_warnings
    GTest::gtest_main
    ml
    simdjson
    xxhash
)

gtest_discover_tests(core_tests)

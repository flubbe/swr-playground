add_executable(core_tests
    test_allocators.cpp
    test_logging.cpp
    test_mesh.cpp
    test_task_system.cpp
    test_utils.cpp
    test_viewport.cpp
    test_camera_reflection.cpp
    ${PROJECT_SOURCE_DIR}/src/logging.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/allocators/bump.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/allocators/malloc.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/manager.cpp
    ${PROJECT_SOURCE_DIR}/src/mesh.cpp
    ${PROJECT_SOURCE_DIR}/src/mesh_simplifier.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/dag.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/scheduler.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/state.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/task_system.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/builtin_properties.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/class_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/properties.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/object.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/camera.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/light.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/directionallight.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/spotlight.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/scene.cpp
    ${PROJECT_SOURCE_DIR}/src/viewport.cpp
)

target_include_directories(core_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${concurrency_utils_src_SOURCE_DIR}/include
    $<TARGET_PROPERTY:swrast,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(core_tests PRIVATE
    swr_playground_options
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(core_tests)

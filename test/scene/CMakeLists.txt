add_library(scene_test_common STATIC
    ${PROJECT_SOURCE_DIR}/src/reflection/builtin_properties.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/class_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/properties.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/object.cpp
)

target_include_directories(scene_test_common PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(scene_test_common PRIVATE
    SWR_USE_CUSTOM_STD_ALLOCATORS=0
)

target_link_libraries(scene_test_common PRIVATE
    swr_playground_warnings
    ml
)

add_executable(object_tests
    test_object_snapshot.cpp
)

target_include_directories(object_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(object_tests PRIVATE
    SWR_USE_CUSTOM_STD_ALLOCATORS=0
)

target_link_libraries(object_tests PRIVATE
    swr_playground_warnings
    scene_test_common
    GTest::gtest_main
    ml
)


gtest_discover_tests(object_tests)

add_executable(scene_tests
    test_scene.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/camera.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/light.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/directionallight.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/spotlight.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/scene.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/static_mesh.cpp
    ${PROJECT_SOURCE_DIR}/src/serialization/json/json_property_writer.cpp
    ${PROJECT_SOURCE_DIR}/src/logging.cpp
)

target_include_directories(scene_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(scene_tests PRIVATE
    SWR_USE_CUSTOM_STD_ALLOCATORS=0
)

target_link_libraries(scene_tests PRIVATE
    swr_playground_warnings
    scene_test_common
    GTest::gtest_main
    ml
    simdjson
)

gtest_discover_tests(scene_tests)

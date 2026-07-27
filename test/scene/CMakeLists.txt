add_executable(object_tests
    test_object_snapshot.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/builtin_properties.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/class_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/properties.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/object.cpp
)

target_include_directories(object_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    $<TARGET_PROPERTY:swrast,INTERFACE_INCLUDE_DIRECTORIES>
)

set_target_properties(object_tests PROPERTIES
    SWR_USE_CUSTOM_STD_ALLOCATORS OFF
)

target_link_libraries(object_tests PRIVATE
    swr_playground_options
    GTest::gtest_main
)


gtest_discover_tests(object_tests)

add_executable(scene_tests
    test_scene.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/builtin_properties.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/class_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/properties.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/object.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/camera.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/light.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/directionallight.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/spotlight.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/scene.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/static_mesh.cpp
)

target_include_directories(scene_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    $<TARGET_PROPERTY:swrast,INTERFACE_INCLUDE_DIRECTORIES>
)

set_target_properties(scene_tests PROPERTIES
    SWR_USE_CUSTOM_STD_ALLOCATORS OFF
)

target_link_libraries(scene_tests PRIVATE
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(scene_tests PRIVATE /W4)
else()
    target_compile_options(scene_tests PRIVATE -Wall -Wextra)
endif()

gtest_discover_tests(scene_tests)

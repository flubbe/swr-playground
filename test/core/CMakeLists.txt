add_executable(core_tests
    test_utils.cpp
    test_viewport.cpp
    test_camera_reflection.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/builtin_properties.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/class_registry.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/properties.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/object.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/camera.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/light.cpp
    ${PROJECT_SOURCE_DIR}/src/scene/scene.cpp
)

target_include_directories(core_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    $<TARGET_PROPERTY:swrast,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(core_tests PRIVATE
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(core_tests PRIVATE /W4)
else()
    target_compile_options(core_tests PRIVATE -Wall -Wextra)
endif()

include(GoogleTest)
gtest_discover_tests(core_tests)

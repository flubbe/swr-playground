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

target_link_libraries(object_tests PRIVATE
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(object_tests PRIVATE /W4)
else()
    target_compile_options(object_tests PRIVATE -Wall -Wextra)
endif()

include(GoogleTest)
gtest_discover_tests(object_tests)

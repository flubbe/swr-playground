add_executable(reflection_tests
    test_class_info.cpp
    test_builtin_property.cpp
    test_cast.cpp
    test_reflection.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/builtin_properties.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/class_registry.cpp
)

target_include_directories(reflection_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(reflection_tests PRIVATE
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(reflection_tests PRIVATE /W4)
else()
    target_compile_options(reflection_tests PRIVATE -Wall -Wextra)
endif()

include(GoogleTest)
gtest_discover_tests(reflection_tests)

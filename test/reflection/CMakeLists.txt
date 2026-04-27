add_executable(reflection_tests
    class_info_tests.cpp
    builtin_property_tests.cpp
    reflection_tests.cpp
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

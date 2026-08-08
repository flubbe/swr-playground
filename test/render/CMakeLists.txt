add_executable(render_tests
    test_shader_cache.cpp
)

target_include_directories(render_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${concurrency_utils_src_SOURCE_DIR}/include
)

target_link_libraries(render_tests PRIVATE
    swr_playground_warnings
    GTest::gtest_main
    ml
    swrast
)

gtest_discover_tests(render_tests)

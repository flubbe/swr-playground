add_executable(render_tests
    test_material_manager.cpp
    test_shader_factory.cpp
    ${PROJECT_SOURCE_DIR}/src/assets/material.cpp
    ${PROJECT_SOURCE_DIR}/src/assets/texture.cpp
    ${PROJECT_SOURCE_DIR}/src/renderer/materialmanager.cpp
    ${PROJECT_SOURCE_DIR}/src/renderer/renderdevice.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/dag.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/scheduler.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/state.cpp
    ${PROJECT_SOURCE_DIR}/src/tasks/task_system.cpp
    ${PROJECT_SOURCE_DIR}/src/logging.cpp
    ${PROJECT_SOURCE_DIR}/src/shader_cache.cpp
    ${PROJECT_SOURCE_DIR}/src/texture_cache.cpp
)

target_include_directories(render_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${concurrency_utils_src_SOURCE_DIR}/include
)

target_compile_definitions(render_tests PRIVATE
    SWR_USE_CUSTOM_STD_ALLOCATORS=0
    ASSETS_SOURCE_DIR="${CMAKE_SOURCE_DIR}/assets"
)

target_link_libraries(render_tests PRIVATE
    swr_playground_warnings
    GTest::gtest_main
    Microsoft.GSL::GSL
    ml
    simdjson
    stb
    swrast
    xxhash
)

gtest_discover_tests(render_tests)

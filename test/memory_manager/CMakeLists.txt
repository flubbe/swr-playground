add_executable(memory_manager_tests
    test_memory_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/allocators/arena.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/allocators/bump.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/allocators/malloc.cpp
    ${PROJECT_SOURCE_DIR}/src/memory/manager.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/builtin_properties.cpp
    ${PROJECT_SOURCE_DIR}/src/reflection/class_registry.cpp
)

target_include_directories(memory_manager_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(memory_manager_tests PRIVATE
    SWR_USE_CUSTOM_STD_ALLOCATORS=1
)

target_link_libraries(memory_manager_tests PRIVATE
    swr_playground_warnings
    GTest::gtest_main
)

gtest_discover_tests(memory_manager_tests)

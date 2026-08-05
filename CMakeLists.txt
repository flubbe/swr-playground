cmake_minimum_required(VERSION 3.25)

# Use Boost package config mode; FindBoost is removed in newer CMake versions.
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(swr_playground LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

option(SWR_PLAYGROUND_BUILD_TESTING "Build swr_playground tests" ON)
option(SWR_USE_CUSTOM_STD_ALLOCATORS "Use custom allocator for std containers" ON)
option(SWR_OVERRIDE_GLOBAL_NEW "Route global new/delete call through the memory manager" ON)

# Keep third-party dependency tests disabled.
set(BUILD_TESTING OFF)
set(BUILD_TESTING OFF CACHE BOOL "Disable dependency tests" FORCE)

find_package(OpenGL REQUIRED)

find_package(Boost CONFIG REQUIRED)
if(TARGET Boost::headers)
    set(BOOST_HEADERS_TARGET Boost::headers)
elseif(TARGET Boost::boost)
    set(BOOST_HEADERS_TARGET Boost::boost)
else()
    message(FATAL_ERROR "Boost target is not available.")
endif()

set_target_properties(${BOOST_HEADERS_TARGET} PROPERTIES SYSTEM TRUE)

#
# Guidelines Support Library
#
FetchContent_Declare(
    gsl
    GIT_REPOSITORY https://github.com/microsoft/GSL.git
    GIT_TAG v4.2.1
    SYSTEM
)

FetchContent_MakeAvailable(gsl)

#
# SDL3
#
find_package(SDL3 QUIET CONFIG)
if(NOT TARGET SDL3::SDL3)
    message(STATUS "SDL3 not found on system; fetching SDL3.")
    FetchContent_Declare(
        SDL3_src
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.4
        SYSTEM
    )
    FetchContent_MakeAvailable(SDL3_src)
endif()

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "SDL3::SDL3 target is not available.")
endif()

#
# SDL3_ttf
#
find_package(SDL3_ttf QUIET CONFIG)
if(NOT TARGET SDL3_ttf::SDL3_ttf)
    message(STATUS "SDL3_ttf not found on system; fetching SDL3_ttf.")
    FetchContent_Declare(
        SDL3_ttf_src
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-3.2.2
        SYSTEM
    )
    FetchContent_MakeAvailable(SDL3_ttf_src)
endif()

if(NOT TARGET SDL3_ttf::SDL3_ttf)
    message(FATAL_ERROR "SDL3_ttf::SDL3_ttf target is not available.")
endif()

#
# SDL3_image
#
find_package(SDL3_image QUIET CONFIG)
if(NOT TARGET SDL3_image::SDL3_image)
    message(STATUS "SDL3_image not found on system; fetching SDL3_image.")
    FetchContent_Declare(
        SDL3_image_src
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG release-3.4.4
        SYSTEM
    )
    FetchContent_MakeAvailable(SDL3_image_src)

    get_target_property(SDL3_IMAGE_REAL_TARGET SDL3_image::SDL3_image ALIASED_TARGET)
    if(NOT SDL3_IMAGE_REAL_TARGET)
        set(SDL3_IMAGE_REAL_TARGET SDL3_image::SDL3_image)
    endif()

    if(TARGET ${SDL3_IMAGE_REAL_TARGET} AND CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        target_compile_options(${SDL3_IMAGE_REAL_TARGET} PRIVATE -Wno-deprecated-declarations)
    endif()
endif()

if(TARGET SDL3_image::SDL3_image)
    get_target_property(SDL3_IMAGE_REAL_TARGET SDL3_image::SDL3_image ALIASED_TARGET)

    if(SDL3_IMAGE_REAL_TARGET)
        set_target_properties(${SDL3_IMAGE_REAL_TARGET} PROPERTIES SYSTEM TRUE)
    else()
        set_target_properties(SDL3_image::SDL3_image PROPERTIES SYSTEM TRUE)
    endif()
else()
    message(FATAL_ERROR "SDL3_image::SDL3_image target is not available.")
endif()

#
# swr
#
set(SWR_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_APP OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SWR_RASTERIZER_BLOCK_SHIFT "5" CACHE STRING "" FORCE)
set(SWR_ENABLE_MULTI_THREADING ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    swr_src
    GIT_REPOSITORY https://github.com/flubbe/swr.git
    GIT_TAG feature/update-ml
    SYSTEM
)
set(FETCHCONTENT_UPDATES_DISCONNECTED_SWR_SRC OFF)
FetchContent_MakeAvailable(swr_src)

#
# ImGui
#
FetchContent_Declare(
    imgui_src
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    SYSTEM
)
FetchContent_MakeAvailable(imgui_src)

add_library(imgui STATIC
    ${imgui_src_SOURCE_DIR}/imgui.cpp
    ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_src_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_src_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_src_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_src_SOURCE_DIR}
    ${imgui_src_SOURCE_DIR}/backends
)

target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_DOCKING)

target_link_libraries(imgui PUBLIC
    SDL3::SDL3
    OpenGL::GL
)

#
# stb
#
FetchContent_Declare(
    stb_src
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 28d546d
    SOURCE_SUBDIR __unused__
    SYSTEM
)

FetchContent_MakeAvailable(stb_src)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE
    "${stb_src_SOURCE_DIR}"
)

#
# Assimp
#
find_package(assimp QUIET CONFIG)
if(NOT TARGET assimp::assimp)
    message(STATUS "assimp not found on system; fetching assimp.")

    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)

    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)

    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        assimp_src
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.5
        SYSTEM
    )

    FetchContent_MakeAvailable(assimp_src)
endif()

if(NOT TARGET assimp::assimp)
    message(FATAL_ERROR "assimp::assimp target is not available.")
endif()

include(FetchContent)

FetchContent_Declare(
    xxhash
    GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
    GIT_TAG v0.8.3
)

FetchContent_GetProperties(xxhash)
if(NOT xxhash_POPULATED)
    FetchContent_MakeAvailable(xxhash)

    add_library(xxhash STATIC
        ${xxhash_SOURCE_DIR}/xxhash.c
    )

    target_include_directories(xxhash
        SYSTEM PUBLIC
        ${xxhash_SOURCE_DIR}
    )
endif()

#
# Concurrency Utils
#
FetchContent_Declare(
    concurrency_utils_src
    GIT_REPOSITORY https://github.com/flubbe/concurrency_utils.git
    GIT_TAG main
    SYSTEM
)
FetchContent_MakeAvailable(concurrency_utils_src)

#
# Compiler specific warnings.
#
add_library(swr_playground_warnings INTERFACE)

if(MSVC)
    target_compile_options(swr_playground_warnings INTERFACE /W4)
else()
    target_compile_options(swr_playground_warnings INTERFACE -Wall -Wextra)
endif()

#
# Playground options.
#
add_library(swr_playground_options INTERFACE)

target_compile_features(swr_playground_options
    INTERFACE
    cxx_std_23)

target_link_libraries(swr_playground_options
    INTERFACE
    ${BOOST_HEADERS_TARGET})

target_compile_definitions(swr_playground_options INTERFACE
    SWR_USE_CUSTOM_STD_ALLOCATORS=$<BOOL:${SWR_USE_CUSTOM_STD_ALLOCATORS}>
    SWR_OVERRIDE_GLOBAL_NEW=$<BOOL:${SWR_OVERRIDE_GLOBAL_NEW}>
)

#
# Playground executable
#
add_executable(swr_playground
    src/assets/static_mesh_importer.cpp
    src/assets/texture.cpp
    src/reflection/builtin_properties.cpp
    src/reflection/class_registry.cpp
    src/scene/camera.cpp
    src/scene/directionallight.cpp
    src/scene/gear.cpp
    src/scene/light.cpp
    src/scene/object.cpp
    src/scene/properties.cpp
    src/scene/scene.cpp
    src/serialization/file.cpp
    src/scene/spotlight.cpp
    src/scene/static_mesh.cpp
    src/memory/allocators/arena.cpp
    src/memory/allocators/bump.cpp
    src/memory/allocators/malloc.cpp
    src/memory/manager.cpp
    src/memory/new.cpp
    src/meshes/mesh.cpp
    src/meshes/simplifier.cpp
    src/renderer/renderdevice.cpp
    src/renderer/renderer.cpp
    src/ui/class_inspector.cpp
    src/ui/console.cpp
    src/ui/imgui.cpp
    src/ui/memory_profiler.cpp
    src/ui/profiler.cpp
    src/ui/scene_inspector.cpp
    src/ui/tools.cpp
    src/application.cpp
    src/logging.cpp
    src/main.cpp
    src/main_loop.cpp
    src/platform.cpp
    src/startup_tasks.cpp
    src/tasks/dag.cpp
    src/tasks/scheduler.cpp
    src/tasks/state.cpp
    src/tasks/task_system.cpp
    src/viewport.cpp
)

target_include_directories(swr_playground PRIVATE
    src
    ${concurrency_utils_src_SOURCE_DIR}/include
)

target_link_libraries(swr_playground PRIVATE
    swr_playground_options
    swr_playground_warnings
    swrast
    imgui
    Microsoft.GSL::GSL
    OpenGL::GL
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
    assimp::assimp
    stb
    xxhash
)

# Enable this project's tests (and CTest) after dependency setup.
set(BUILD_TESTING ${SWR_PLAYGROUND_BUILD_TESTING})
set(BUILD_TESTING ${SWR_PLAYGROUND_BUILD_TESTING} CACHE BOOL "Build tests" FORCE)
include(CTest)

if(BUILD_TESTING)
    #
    # GoogleTest
    #
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        SYSTEM
    )
    FetchContent_MakeAvailable(googletest)

    if(TARGET GTest::gtest_main)
        get_target_property(GTEST_REAL_TARGET GTest::gtest_main ALIASED_TARGET)
        if(GTEST_REAL_TARGET)
            set_target_properties(${GTEST_REAL_TARGET} PROPERTIES SYSTEM TRUE)
        else()
            set_target_properties(GTest::gtest_main PROPERTIES SYSTEM TRUE)
        endif()
    endif()

    add_subdirectory(test)
endif()

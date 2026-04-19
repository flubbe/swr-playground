cmake_minimum_required(VERSION 3.24)
project(swr_playground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(OpenGL REQUIRED)

#
# Guidelines Support Library
#
FetchContent_Declare(
    gsl
    GIT_REPOSITORY https://github.com/microsoft/GSL.git
    GIT_TAG v4.2.1
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
    )
    FetchContent_MakeAvailable(SDL3_src)
endif()

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "SDL3::SDL3 target is not available.")
endif()

#
# swr
#
set(SWR_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_APP OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    swr_src
    GIT_REPOSITORY https://github.com/flubbe/swr.git
    GIT_TAG 7df418f
)
FetchContent_MakeAvailable(swr_src)

#
# ImGui
#
FetchContent_Declare(
    imgui_src
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui_src)

add_library(imgui STATIC
    ${imgui_src_SOURCE_DIR}/imgui.cpp
    ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_src_SOURCE_DIR}/imgui_demo.cpp
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
# Playground executable
#
add_executable(swr_playground
    src/scene/gear.cpp
    src/scene/scene.cpp
    src/main.cpp
    src/platform.cpp
    src/renderdevice.cpp
    src/renderer.cpp
)

target_include_directories(swr_playground PRIVATE src)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    target_compile_options(swr_playground PRIVATE
        -msse
        -msse2
        -msse3
        -msse4
        -msse4.1
        -msse4.2
        -mfpmath=sse
    )
endif()

target_link_libraries(swr_playground PRIVATE
    swrast
    imgui
    Microsoft.GSL::GSL
    OpenGL::GL
    SDL3::SDL3
)

if(MSVC)
    target_compile_options(swr_playground PRIVATE /W4)
else()
    target_compile_options(swr_playground PRIVATE -Wall -Wextra)
endif()

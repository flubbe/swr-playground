cmake_minimum_required(VERSION 3.24)
project(swr_playground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(SDL3 REQUIRED CONFIG)
find_package(OpenGL REQUIRED)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_DOCKING)
target_link_libraries(imgui PUBLIC SDL3::SDL3 OpenGL::GL)

set(SWR_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_APP OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(SWR_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    swr_src
    GIT_REPOSITORY https://github.com/flubbe/swr.git
    GIT_TAG 3fd648f
)
FetchContent_MakeAvailable(swr_src)

add_executable(swr_playground
    src/main.cpp
    src/framebuffer.cpp
)

target_include_directories(swr_playground PRIVATE src)

target_link_libraries(swr_playground PRIVATE
    swrast
    imgui
    SDL3::SDL3
    OpenGL::GL
)

if(MSVC)
    target_compile_options(swr_playground PRIVATE /W4)
else()
    target_compile_options(swr_playground PRIVATE -Wall -Wextra)
endif()

cmake_minimum_required(VERSION 3.18)
project(QhenkiX-Workspace)

# Set repo root for all subprojects
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(SDL3_VERSION "3.2.28")

option(BUILD_GLTF_VIEWER "Build gltfViewer example" ON)
option(BUILD_IMGUI_EXAMPLE "Build ImGuiExample" ON)
option(BUILD_SIMPLE_EXAMPLE "Build SimpleExample" ON)

add_subdirectory(QhenkiX)
add_subdirectory(SXC)

if(BUILD_GLTF_VIEWER)
    add_subdirectory(Examples/gltfViewer)
endif()

if(BUILD_IMGUI_EXAMPLE)
    add_subdirectory(Examples/ImGuiExample)
endif()

if(BUILD_SIMPLE_EXAMPLE)
    add_subdirectory(Examples/SimpleExample)
endif()

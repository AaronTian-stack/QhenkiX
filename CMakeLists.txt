cmake_minimum_required(VERSION 3.18)
project(QhenkiX-Workspace)

# Set repo root for all subprojects
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(QhenkiX)
add_subdirectory(SXC)
add_subdirectory(Examples/gltfViewer)
add_subdirectory(Examples/ImGuiExample)
add_subdirectory(Examples/SimpleExample)

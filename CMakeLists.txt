cmake_minimum_required(VERSION 3.18)
project(QhenkiX-Workspace)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeOverride.txt)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set repo root for all subprojects
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(SDL3_VERSION "3.2.28")

option(BUILD_GLTF_VIEWER "Build gltfViewer example" ON)
option(BUILD_IMGUI_EXAMPLE "Build ImGuiExample" ON)
option(BUILD_SIMPLE_EXAMPLE "Build SimpleExample" ON)

set(QHENKIX_PREBUILT_LIB "" CACHE FILEPATH "Path to prebuilt QhenkiX.lib (CI only)")
set(SXC_PREBUILT_EXE "" CACHE FILEPATH "Path to prebuilt SXC.exe (CI only)")

if(QHENKIX_PREBUILT_LIB)
    include(QhenkiX/QhenkiXDeps.cmake)
    
    add_library(QhenkiX STATIC IMPORTED GLOBAL)
    set_target_properties(QhenkiX PROPERTIES IMPORTED_LOCATION "${QHENKIX_PREBUILT_LIB}")
    target_include_directories(QhenkiX INTERFACE ${QHENKIX_INCLUDE_DIRS})
    
    if(WIN32)
        qhenkix_setup_DirectXTex()
        target_link_libraries(QhenkiX INTERFACE
            DirectXTex
            ${QHENKIX_SYSTEM_LIBS}
            "${QHENKIX_SDL3_LIB}"
        )
    endif()
else()
    add_subdirectory(QhenkiX)
endif()

if(SXC_PREBUILT_EXE)
    add_executable(SXC IMPORTED GLOBAL)
    set_target_properties(SXC PROPERTIES IMPORTED_LOCATION "${SXC_PREBUILT_EXE}")
else()
    add_subdirectory(SXC)
endif()

if(BUILD_GLTF_VIEWER)
    add_subdirectory(Examples/gltfViewer)
endif()

if(BUILD_IMGUI_EXAMPLE)
    add_subdirectory(Examples/ImGuiExample)
endif()

if(BUILD_SIMPLE_EXAMPLE)
    add_subdirectory(Examples/SimpleExample)
endif()

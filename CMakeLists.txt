cmake_minimum_required(VERSION 3.18)
project(QhenkiX-Workspace)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set repo root for all subprojects
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(SDL3_VERSION "3.2.28")

option(BUILD_GLTF_VIEWER "Build gltfViewer example" ON)
option(BUILD_IMGUI_EXAMPLE "Build ImGuiExample" ON)
option(BUILD_SIMPLE_EXAMPLE "Build SimpleExample" ON)

# Option for CI: use pre-built QhenkiX library instead of building from source
set(QHENKIX_PREBUILT_LIB "" CACHE FILEPATH "Path to pre-built QhenkiX.lib (CI only)")

if(QHENKIX_PREBUILT_LIB)
    # Use pre-built library - create an imported target
    include(QhenkiX/QhenkiXDeps.cmake)
    
    add_library(QhenkiX STATIC IMPORTED GLOBAL)
    set_target_properties(QhenkiX PROPERTIES IMPORTED_LOCATION "${QHENKIX_PREBUILT_LIB}")
    target_include_directories(QhenkiX INTERFACE ${QHENKIX_INCLUDE_DIRS})
    
    if(WIN32)
        qhenkix_setup_directxtex()
        target_link_libraries(QhenkiX INTERFACE
            DirectXTex
            ${QHENKIX_SYSTEM_LIBS}
            "${QHENKIX_SDL3_LIB}"
        )
    endif()
else()
    # Build from source
    add_subdirectory(QhenkiX)
endif()

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

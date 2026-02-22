cmake_minimum_required(VERSION 3.21)
project(QhenkiX-Workspace)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeOverride.txt)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set repo root for all subprojects
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

option(BUILD_GLTF_VIEWER "Build gltfViewer example" ON)
option(BUILD_IMGUI_EXAMPLE "Build ImGuiExample" ON)
option(BUILD_SIMPLE_EXAMPLE "Build SimpleExample" ON)
option(BUILD_RETRO_EXAMPLE "Build RetroExample" ON)

set(QHENKIX_PREBUILT_LIB "" CACHE FILEPATH "Path to prebuilt QhenkiX.lib (CI only)")
set(QHENKIX_PREBUILT_SDL_ROOT "" CACHE PATH "Path to prebuilt SDL3 output")
set(SXC_PREBUILT_EXE "" CACHE FILEPATH "Path to prebuilt SXC.exe (CI only)")

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_X11 ON CACHE BOOL "" FORCE)
set(SDL_WAYLAND ON CACHE BOOL "" FORCE)
set(SDL_Hidapi OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

# Turn off DirectXTex runtime stuff
set(BUILD_DX11 OFF CACHE BOOL "" FORCE)
set(BUILD_DX12 OFF CACHE BOOL "" FORCE)

if(QHENKIX_PREBUILT_LIB)
    include(QhenkiX/QhenkiXDeps.cmake)
    set(DXHEADERS_INSTALL ON CACHE BOOL "" FORCE)
    set(DXHEADERS_BUILD_TEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(QhenkiX/external/DirectX-Headers ${CMAKE_BINARY_DIR}/DirectX-Headers)
    set(directx-headers_DIR "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/cmake" CACHE PATH "" FORCE)

    set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
    set(_qhenkix_prev_build_shared_libs "")
    if(DEFINED BUILD_SHARED_LIBS)
        set(_qhenkix_prev_build_shared_libs "${BUILD_SHARED_LIBS}")
    endif()
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(QhenkiX/external/DirectXTex ${CMAKE_BINARY_DIR}/DirectXTex)
    if(_qhenkix_prev_build_shared_libs STREQUAL "")
        unset(BUILD_SHARED_LIBS)
    else()
        set(BUILD_SHARED_LIBS "${_qhenkix_prev_build_shared_libs}")
    endif()

    if(QHENKIX_PREBUILT_SDL_ROOT)
        set(_qhenkix_sdl_search_paths)
        foreach(_qhenkix_cfg Debug Release RelWithDebInfo MinSizeRel)
            list(APPEND _qhenkix_sdl_search_paths "${QHENKIX_PREBUILT_SDL_ROOT}/build/SDL/${_qhenkix_cfg}")
        endforeach()

        find_file(_qhenkix_sdl_lib SDL3.lib
            PATHS ${_qhenkix_sdl_search_paths}
            NO_DEFAULT_PATH
        )

        find_file(_qhenkix_sdl_dll SDL3.dll
            PATHS ${_qhenkix_sdl_search_paths}
            NO_DEFAULT_PATH
        )

        set(_qhenkix_sdl_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/external/SDL/include")

        if(NOT _qhenkix_sdl_lib)
            message(FATAL_ERROR
                "QHENKIX_PREBUILT_SDL_ROOT is set to '${QHENKIX_PREBUILT_SDL_ROOT}', but SDL3.lib was not found. Checked: ${_qhenkix_sdl_search_paths}"
            )
        endif()
        if(NOT _qhenkix_sdl_dll)
            message(FATAL_ERROR
                "QHENKIX_PREBUILT_SDL_ROOT is set to '${QHENKIX_PREBUILT_SDL_ROOT}', but SDL3.dll was not found. Checked: ${_qhenkix_sdl_search_paths}"
            )
        endif()
        if(NOT EXISTS "${_qhenkix_sdl_include_dir}/SDL3/SDL.h")
            message(FATAL_ERROR
                "Missing SDL headers at '${_qhenkix_sdl_include_dir}'. Ensure submodules are initialized with: git submodule update --init --recursive"
            )
        endif()

        add_library(SDL3::SDL3 SHARED IMPORTED GLOBAL)
        set_target_properties(SDL3::SDL3 PROPERTIES
            IMPORTED_IMPLIB "${_qhenkix_sdl_lib}"
            IMPORTED_LOCATION "${_qhenkix_sdl_dll}"
            INTERFACE_INCLUDE_DIRECTORIES "${_qhenkix_sdl_include_dir}"
        )
    else()
        add_subdirectory(QhenkiX/external/SDL ${CMAKE_BINARY_DIR}/SDL)
    endif()

    add_library(QhenkiX STATIC IMPORTED GLOBAL)
    set_target_properties(QhenkiX PROPERTIES IMPORTED_LOCATION "${QHENKIX_PREBUILT_LIB}")
    target_include_directories(QhenkiX INTERFACE ${QHENKIX_INCLUDE_DIRS})
    target_link_libraries(QhenkiX INTERFACE DirectXTex SDL3::SDL3)
    if(WIN32)
        target_link_libraries(QhenkiX INTERFACE ${QHENKIX_SYSTEM_LIBS})
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

if(BUILD_RETRO_EXAMPLE)
    add_subdirectory(Examples/RetroExample)
endif()

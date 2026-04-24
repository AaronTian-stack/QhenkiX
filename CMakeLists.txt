cmake_minimum_required(VERSION 3.21)
project(QhenkiX-Workspace)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeOverride.txt)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/cmake/ScopedBuildSharedLibs.cmake)

# Set repo root for all subprojects
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

option(BUILD_GLTF_VIEWER "Build gltfViewer example" ON)
option(BUILD_IMGUI_EXAMPLE "Build ImGuiExample" ON)
option(BUILD_SIMPLE_EXAMPLE "Build SimpleExample" ON)
option(BUILD_RETRO_EXAMPLE "Build RetroExample" ON)

# Mainly intended for CI purposes but could technically be used by users as well
set(QHENKIX_PREBUILT_LIB "" CACHE FILEPATH "Path to prebuilt QhenkiX.lib")
set(QHENKIX_PREBUILT_SDL_ROOT "" CACHE PATH "Path to prebuilt SDL3 output")
set(QHENKIX_PREBUILT_DIRECTXTEX_LIB "" CACHE FILEPATH "Path to prebuilt DirectXTex.lib")
set(QHENKIX_PREBUILT_DIRECTXGUIDS_LIB "" CACHE FILEPATH "Path to prebuilt DirectX-Guids.lib")
set(SXC_PREBUILT_EXE "" CACHE FILEPATH "Path to prebuilt SXC.exe")

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

function(qhenkix_require_existing _path _var_name)
    if(NOT EXISTS "${_path}")
        message(FATAL_ERROR
            "${_var_name} is set to '${_path}', but the file does not exist."
        )
    endif()
endfunction()

function(qhenkix_setup_directx_dependencies)
    if(QHENKIX_PREBUILT_DIRECTXTEX_LIB AND QHENKIX_PREBUILT_DIRECTXGUIDS_LIB)
        qhenkix_require_existing("${QHENKIX_PREBUILT_DIRECTXTEX_LIB}" "QHENKIX_PREBUILT_DIRECTXTEX_LIB")
        qhenkix_require_existing("${QHENKIX_PREBUILT_DIRECTXGUIDS_LIB}" "QHENKIX_PREBUILT_DIRECTXGUIDS_LIB")

        add_library(DirectX-Guids STATIC IMPORTED GLOBAL)
        set_target_properties(DirectX-Guids PROPERTIES
            IMPORTED_LOCATION "${QHENKIX_PREBUILT_DIRECTXGUIDS_LIB}"
        )

        add_library(DirectXTex STATIC IMPORTED GLOBAL)
        set_target_properties(DirectXTex PROPERTIES
            IMPORTED_LOCATION "${QHENKIX_PREBUILT_DIRECTXTEX_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/external/DirectXTex/DirectXTex"
        )
        target_link_libraries(DirectXTex INTERFACE DirectX-Guids)

        add_library(DirectXMath INTERFACE IMPORTED GLOBAL)
        set_target_properties(DirectXMath PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/external/DirectXMath/Inc"
            INTERFACE_COMPILE_DEFINITIONS "_XM_NO_XMVECTOR_OVERLOADS_"
        )
        if(NOT TARGET Microsoft::DirectXMath)
            add_library(Microsoft::DirectXMath ALIAS DirectXMath)
        endif()
    else()
        qhenkix_add_dependency_subdirectory(QhenkiX/external/DirectXMath ${CMAKE_BINARY_DIR}/DirectXMath)
        if(TARGET DirectXMath AND NOT TARGET Microsoft::DirectXMath)
            add_library(Microsoft::DirectXMath ALIAS DirectXMath)
        endif()
        if(TARGET DirectXMath)
            target_compile_definitions(DirectXMath INTERFACE _XM_NO_XMVECTOR_OVERLOADS_)
        endif()
        set(directxmath_DIR "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/cmake" CACHE PATH "" FORCE)

        set(DXHEADERS_INSTALL ON CACHE BOOL "" FORCE)
        set(DXHEADERS_BUILD_TEST OFF CACHE BOOL "" FORCE)
        qhenkix_add_dependency_subdirectory(QhenkiX/external/DirectX-Headers ${CMAKE_BINARY_DIR}/DirectX-Headers)
        set(directx-headers_DIR "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/cmake" CACHE PATH "" FORCE)

        set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
        set(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
        qhenkix_add_subdirectory_with_build_shared_libs(QhenkiX/external/DirectXTex ${CMAKE_BINARY_DIR}/DirectXTex OFF)
    endif()

    if(TARGET DirectXMath)
        target_include_directories(DirectXMath INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/external")
    endif()
endfunction()

function(qhenkix_setup_sdl_dependency)
    if(QHENKIX_PREBUILT_SDL_ROOT)
        set(_qhenkix_sdl_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/external/SDL/include")
        if(NOT EXISTS "${_qhenkix_sdl_include_dir}/SDL3/SDL.h")
            message(FATAL_ERROR
                "Missing SDL headers at '${_qhenkix_sdl_include_dir}'. Ensure submodules are initialized with: git submodule update --init --recursive"
            )
        endif()

        if(WIN32)
            set(_qhenkix_sdl_search_paths)
            foreach(_qhenkix_cfg Debug Release RelWithDebInfo MinSizeRel)
                list(APPEND _qhenkix_sdl_search_paths "${QHENKIX_PREBUILT_SDL_ROOT}/SDL/${_qhenkix_cfg}")
            endforeach()

            find_file(_qhenkix_sdl_lib SDL3.lib
                PATHS ${_qhenkix_sdl_search_paths}
                NO_DEFAULT_PATH
            )

            find_file(_qhenkix_sdl_dll SDL3.dll
                PATHS ${_qhenkix_sdl_search_paths}
                NO_DEFAULT_PATH
            )

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

            add_library(SDL3::SDL3 SHARED IMPORTED GLOBAL)
            set_target_properties(SDL3::SDL3 PROPERTIES
                IMPORTED_IMPLIB "${_qhenkix_sdl_lib}"
                IMPORTED_LOCATION "${_qhenkix_sdl_dll}"
                INTERFACE_INCLUDE_DIRECTORIES "${_qhenkix_sdl_include_dir}"
            )
        elseif(LINUX)
            file(GLOB _qhenkix_sdl_sos
                CONFIGURE_DEPENDS
                "${QHENKIX_PREBUILT_SDL_ROOT}/SDL/libSDL3.so*"
            )
            set(_qhenkix_sdl_so "")
            if(_qhenkix_sdl_sos)
                list(SORT _qhenkix_sdl_sos)
                list(LENGTH _qhenkix_sdl_sos _qhenkix_sdl_sos_n)
                math(EXPR _qhenkix_sdl_sos_last "${_qhenkix_sdl_sos_n} - 1")
                list(GET _qhenkix_sdl_sos ${_qhenkix_sdl_sos_last} _qhenkix_sdl_so)
            endif()
            if(NOT _qhenkix_sdl_so)
                message(FATAL_ERROR
                    "QHENKIX_PREBUILT_SDL_ROOT is set to '${QHENKIX_PREBUILT_SDL_ROOT}', but no libSDL3.so* was found in ${QHENKIX_PREBUILT_SDL_ROOT}/SDL"
                )
            endif()

            add_library(SDL3::SDL3 SHARED IMPORTED GLOBAL)
            set_target_properties(SDL3::SDL3 PROPERTIES
                IMPORTED_LOCATION "${_qhenkix_sdl_so}"
                INTERFACE_INCLUDE_DIRECTORIES "${_qhenkix_sdl_include_dir}"
            )
        else()
            message(FATAL_ERROR "Not supported")
        endif()
    else()
        qhenkix_add_dependency_subdirectory(QhenkiX/external/SDL ${CMAKE_BINARY_DIR}/SDL)
    endif()
endfunction()

function(qhenkix_setup_prebuilt_workspace_dependencies)
    qhenkix_setup_directx_dependencies()
    qhenkix_setup_sdl_dependency()

    set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(VULKAN_HEADERS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    qhenkix_add_dependency_subdirectory(QhenkiX/external/Vulkan-Headers ${CMAKE_BINARY_DIR}/Vulkan-Headers)

    set(VULKAN_HEADERS_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/QhenkiX/external/Vulkan-Headers" CACHE PATH "" FORCE)
    set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)
    set(VOLK_HEADERS_ONLY OFF CACHE BOOL "" FORCE)
    set(VOLK_PULL_IN_VULKAN ON CACHE BOOL "" FORCE)
    qhenkix_add_dependency_subdirectory(QhenkiX/external/volk ${CMAKE_BINARY_DIR}/volk)

    set(VK_BOOTSTRAP_TEST OFF CACHE BOOL "" FORCE)
    set(VK_BOOTSTRAP_INSTALL OFF CACHE BOOL "" FORCE)
    qhenkix_add_dependency_subdirectory(QhenkiX/external/vk-bootstrap ${CMAKE_BINARY_DIR}/vk-bootstrap)

    set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
    qhenkix_add_dependency_subdirectory(QhenkiX/external/SPIRV-Cross ${CMAKE_BINARY_DIR}/spirv-cross)
endfunction()

if(QHENKIX_PREBUILT_LIB)
    include(QhenkiX/QhenkiXDeps.cmake)
    qhenkix_setup_prebuilt_workspace_dependencies()

    add_library(QhenkiX STATIC IMPORTED GLOBAL)
    set_target_properties(QhenkiX PROPERTIES IMPORTED_LOCATION "${QHENKIX_PREBUILT_LIB}")
    target_include_directories(QhenkiX INTERFACE ${QHENKIX_INCLUDE_DIRS})
    target_link_libraries(QhenkiX INTERFACE
        DirectXMath
        DirectXTex
        DirectX-Guids
        SDL3::SDL3
        vk-bootstrap::vk-bootstrap
        spirv-cross-core
        spirv-cross-glsl
    )
    if(QHENKIX_SYSTEM_LIBS)
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

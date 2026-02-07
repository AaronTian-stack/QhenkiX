cmake_minimum_required(VERSION 3.18)
project(QhenkiX LANGUAGES CXX)

if(NOT SDL3_VERSION)
    set(SDL3_VERSION "3.2.28")
endif()

set(QHENKIX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qhenki")

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
file(GLOB IMGUI_SOURCES "${IMGUI_DIR}/*.cpp")
set(QHENKIX_SOURCES
    "${QHENKIX_DIR}/application.cpp"
    "${QHENKIX_DIR}/graphics/camera.cpp"
    "${QHENKIX_DIR}/graphics/display_window.cpp"
    "${QHENKIX_DIR}/graphics/orthographic_camera_component.cpp"
    "${QHENKIX_DIR}/graphics/perspective_camera_component.cpp"
    "${QHENKIX_DIR}/graphics/2d/spritebatch.cpp"

    "${QHENKIX_DIR}/graphics/d3d11/d3d11_context.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_layout_assembler.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_multithread.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_pipeline.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/fxc_include_handler.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/fxc_shader_compiler.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_shader.cpp"

    "${QHENKIX_DIR}/graphics/d3d12/d3d12_context.cpp"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_descriptor_heap.cpp"
    "${QHENKIX_DIR}/graphics/d3d12/dxc_include_handler.cpp"
    "${QHENKIX_DIR}/graphics/d3d12/dxc_shader_compiler.cpp"

    "${QHENKIX_DIR}/input/input_manager.cpp"

    "${QHENKIX_DIR}/math/transform.cpp"
    "${QHENKIX_DIR}/math/transform_simd.cpp"

    "${QHENKIX_DIR}/memory/arena.cpp"

    "${QHENKIX_DIR}/utility/d3d_util.cpp"
    "${QHENKIX_DIR}/utility/file_util.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/external/D3D12MemAlloc/D3D12MemAlloc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/DirectX-Headers-1.618.2/src/dxguids.cpp"
)

set(QHENKIX_PRIVATE_HEADERS
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_context.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_heap.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_layout_assembler.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_multithread.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_pipeline.h"
    "${QHENKIX_DIR}/utility/fxc_include_handler.h"
    "${QHENKIX_DIR}/graphics/d3d11/fxc_shader_compiler.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_shader.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_context.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_descriptor_heap.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_fence.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_pipeline.h"
    "${QHENKIX_DIR}/graphics/d3d12/dxc_include_handler.h"
    "${QHENKIX_DIR}/graphics/d3d12/dxc_shader_compiler.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_texture.h"
    "${QHENKIX_DIR}/utility/d3d_util.h"
)

set(QHENKIX_PUBLIC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/qhenki")

set(QHENKIX_PUBLIC_HEADERS
    "${QHENKIX_PUBLIC_DIR}/application.h"
    "${QHENKIX_PUBLIC_DIR}/camera.h"
    "${QHENKIX_PUBLIC_DIR}/display_window.h"
    "${QHENKIX_PUBLIC_DIR}/orthographic_camera_component.h"
    "${QHENKIX_PUBLIC_DIR}/perspective_camera_component.h"

    "${QHENKIX_PUBLIC_DIR}/2d/spritebatch.h"

    "${QHENKIX_PUBLIC_DIR}/utility/file_util.h"
    "${QHENKIX_PUBLIC_DIR}/utility/general_util.h"
    "${QHENKIX_PUBLIC_DIR}/utility/math_util.h"
    "${QHENKIX_PUBLIC_DIR}/utility/string_util.h"

    "${QHENKIX_PUBLIC_DIR}/input/input_manager.h"

    "${QHENKIX_PUBLIC_DIR}/math/transform.h"
    "${QHENKIX_PUBLIC_DIR}/math/transform_simd.h"

    "${QHENKIX_PUBLIC_DIR}/memory/arena.h"

    "${QHENKIX_PUBLIC_DIR}/RHI/barrier.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/buffer.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/command_list.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/command_pool.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/context.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/descriptor_heap.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/descriptor_table.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/enums.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/pipeline.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/queue.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/render_target.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/sampler.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/shader_compiler.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/shader.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/submission.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/subresource.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/swapchain.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/sync.h"
    "${QHENKIX_PUBLIC_DIR}/RHI/texture.h"

    "${QHENKIX_PUBLIC_DIR}/utility/shader_blob.h"
    "${QHENKIX_PUBLIC_DIR}/utility/hlsl_compat.h"
)

add_library(${PROJECT_NAME} STATIC ${QHENKIX_SOURCES} ${QHENKIX_PRIVATE_HEADERS} ${QHENKIX_PUBLIC_HEADERS} ${IMGUI_SOURCES})

set(ALL_FILES ${QHENKIX_SOURCES} ${QHENKIX_PRIVATE_HEADERS} ${QHENKIX_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_FILES})
source_group("external/ImGui" FILES ${IMGUI_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${QHENKIX_PUBLIC_HEADERS}")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Disable synchronous exceptions
if(MSVC)
    foreach(var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REPLACE "/EHsc" "" ${var} "${${var}}")
    endforeach()
    target_compile_options(${PROJECT_NAME} BEFORE PRIVATE /EHs)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions)
endif()

include(QhenkiXDeps.cmake)

target_include_directories(${PROJECT_NAME} PUBLIC ${QHENKIX_INCLUDE_DIRS}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX UNICODE _UNICODE)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /MP)
    endif()

    qhenkix_setup_DirectXTex()
    target_link_libraries(${PROJECT_NAME} PUBLIC
        DirectXTex
        ${QHENKIX_SYSTEM_LIBS}
        "${QHENKIX_SDL3_LIB}"
    )
endif()

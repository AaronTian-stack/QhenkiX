cmake_minimum_required(VERSION 3.18)
project(QhenkiX LANGUAGES CXX)

set(QHENKIX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qhenkiX")

file(GLOB IMGUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/*.cpp")
set(QHENKIX_SOURCES
    "${QHENKIX_DIR}/application.cpp"
    "${QHENKIX_DIR}/graphics/arcball_controller.cpp"
    "${QHENKIX_DIR}/graphics/camera.cpp"
    "${QHENKIX_DIR}/graphics/display_window.cpp"
    "${QHENKIX_DIR}/graphics/orthographic_camera.cpp"
    "${QHENKIX_DIR}/graphics/perspective_camera.cpp"
    "${QHENKIX_DIR}/graphics/2d/spritebatch.cpp"

    "${QHENKIX_DIR}/graphics/d3d11/d3d11_context.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_layout_assembler.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_pipeline.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_shader_compiler.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_shader.cpp"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_swapchain.cpp"

    "${QHENKIX_DIR}/graphics/d3d12/d3d12_context.cpp"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_descriptor_heap.cpp"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_root_hasher.cpp"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_shader_compiler.cpp"

    "${QHENKIX_DIR}/helper/d3d_helper.cpp"
    "${QHENKIX_DIR}/helper/file_helper.cpp"

    "${QHENKIX_DIR}/input/input_manager.cpp"

    "${QHENKIX_DIR}/math/basis.cpp"
    "${QHENKIX_DIR}/math/transform.cpp"

    "${QHENKIX_DIR}/utility/include_handlers.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/D3D12MemAlloc.cpp"
    ${IMGUI_SOURCES}
)

set(QHENKIX_PRIVATE_HEADERS
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_context.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_heap.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_layout_assembler.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_pipeline.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_shader_compiler.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_shader.h"
    "${QHENKIX_DIR}/graphics/d3d11/d3d11_swapchain.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_context.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_descriptor_heap.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_fence.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_pipeline.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_root_hasher.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_shader_compiler.h"
    "${QHENKIX_DIR}/graphics/d3d12/d3d12_texture.h"
)

set(QHENKIX_PUBLIC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/qhenkiX")

set(QHENKIX_PUBLIC_HEADERS
    "${QHENKIX_PUBLIC_DIR}/application.h"
    "${QHENKIX_PUBLIC_DIR}/arcball_controller.h"
    "${QHENKIX_PUBLIC_DIR}/camera.h"
    "${QHENKIX_PUBLIC_DIR}/display_window.h"
    "${QHENKIX_PUBLIC_DIR}/orthographic_camera.h"
    "${QHENKIX_PUBLIC_DIR}/perspective_camera.h"

    "${QHENKIX_PUBLIC_DIR}/2d/spritebatch.h"

    "${QHENKIX_PUBLIC_DIR}/helper/d3d_helper.h"
    "${QHENKIX_PUBLIC_DIR}/helper/file_helper.h"
    "${QHENKIX_PUBLIC_DIR}/helper/general_helper.h"
    "${QHENKIX_PUBLIC_DIR}/helper/math_helper.h"
    "${QHENKIX_PUBLIC_DIR}/helper/string_helper.h"

    "${QHENKIX_PUBLIC_DIR}/input/input_manager.h"

    "${QHENKIX_PUBLIC_DIR}/math/basis.h"
    "${QHENKIX_PUBLIC_DIR}/math/transform.h"

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

    "${QHENKIX_PUBLIC_DIR}/utility/include_handlers.h"
)

add_library(${PROJECT_NAME} STATIC ${QHENKIX_SOURCES} ${QHENKIX_PRIVATE_HEADERS} ${QHENKIX_PUBLIC_HEADERS})

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR
set(ALL_FILES ${QHENKIX_SOURCES} ${QHENKIX_PRIVATE_HEADERS} ${QHENKIX_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${QHENKIX_PUBLIC_HEADERS}")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /MP) # Enable multi-processor compilation
    endif()
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

    add_library(DirectXTex STATIC IMPORTED)
    set_target_properties(DirectXTex PROPERTIES
        IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/lib/x64/Release/DirectXTex.lib"
        IMPORTED_LOCATION_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/lib/x64/Debug/DirectXTex.lib"
        IMPORTED_LOCATION_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/lib/x64/Release/DirectXTex.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/lib/x64/Release/DirectXTex.lib"
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC
        d3d11
        d3d12
        dxgi
        d3dcompiler
        dxcompiler
        winmm
        dxguid
        DirectXTex
    )

    target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/SDL3-3.2.4/include")
    target_link_libraries(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/SDL3-3.2.4/lib/x64/SDL3.lib")
endif()

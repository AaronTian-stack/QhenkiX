cmake_minimum_required(VERSION 3.21)
project(QhenkiX LANGUAGES CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ScopedBuildSharedLibs.cmake)

function(qhenkix_require_submodule submodule_path expected_file)
    set(_qhenkix_expected_path "${CMAKE_CURRENT_SOURCE_DIR}/${submodule_path}/${expected_file}")
    if(NOT EXISTS "${_qhenkix_expected_path}")
        message(FATAL_ERROR
            "Missing required submodule content at '${_qhenkix_expected_path}'. "
            "Initialize submodules with: git submodule update --init --recursive"
        )
    endif()
endfunction()

qhenkix_require_submodule("external/imgui" "imgui.cpp")
qhenkix_require_submodule("external/DirectXMath" "CMakeLists.txt")
qhenkix_require_submodule("external/DirectX-Headers" "CMakeLists.txt")
qhenkix_require_submodule("external/DirectXTex" "CMakeLists.txt")
qhenkix_require_submodule("external/SDL" "CMakeLists.txt")
qhenkix_require_submodule("external/Vulkan-Headers" "CMakeLists.txt")
qhenkix_require_submodule("external/volk" "CMakeLists.txt")
qhenkix_require_submodule("external/vk-bootstrap" "CMakeLists.txt")
qhenkix_require_submodule("external/SPIRV-Cross" "CMakeLists.txt")

set(QHENKIX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qhenki")

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)
if(WIN32)
    list(APPEND IMGUI_SOURCES
        "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_dx12.cpp"
    )
endif()

set(QHENKIX_SOURCES
    "${QHENKIX_DIR}/application.cpp"
    "${QHENKIX_DIR}/graphics/camera.cpp"
    "${QHENKIX_DIR}/graphics/display_window.cpp"
    "${QHENKIX_DIR}/graphics/orthographic_camera_component.cpp"
    "${QHENKIX_DIR}/graphics/perspective_camera_component.cpp"
    "${QHENKIX_DIR}/graphics/2d/spritebatch.cpp"

    "${QHENKIX_DIR}/graphics/d3d12/dxc_include_handler.cpp"
    "${QHENKIX_DIR}/graphics/d3d12/dxc_shader_compiler.cpp"

    "${QHENKIX_DIR}/graphics/vulkan/vulkan_context.cpp"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_descriptor_heap.cpp"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_command_pool.cpp"

    "${QHENKIX_DIR}/input/input_manager.cpp"

    "${QHENKIX_DIR}/math/transform.cpp"
    "${QHENKIX_DIR}/math/transform_simd.cpp"

    "${QHENKIX_DIR}/memory/arena.cpp"

    "${QHENKIX_DIR}/utility/d3d_reflection_util.cpp"
    "${QHENKIX_DIR}/utility/gfx_util.cpp"
    "${QHENKIX_DIR}/utility/file_util.cpp"
    "${QHENKIX_DIR}/utility/shader_model_util.cpp"
    "${QHENKIX_DIR}/utility/vulkan_util.cpp"

    "${QHENKIX_DIR}/graphics/shared/descriptor_flush.cpp"
)

if(WIN32)
    list(APPEND QHENKIX_SOURCES
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_context.cpp"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_layout_assembler.cpp"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_multithread.cpp"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_pipeline.cpp"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_descriptor_heap.cpp"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_sampler_heap.cpp"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_srv_uav_heap.cpp"
        "${QHENKIX_DIR}/graphics/d3d11/fxc_include_handler.cpp"
        "${QHENKIX_DIR}/graphics/d3d11/fxc_shader_compiler.cpp"

        "${QHENKIX_DIR}/graphics/d3d12/d3d12_context.cpp"
        "${QHENKIX_DIR}/graphics/d3d12/d3d12_descriptor_heap.cpp"

        "${QHENKIX_DIR}/utility/d3d_util.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/external/D3D12MemAlloc/D3D12MemAlloc.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/D3D12DescriptorHelpers/RenderTargetHelper.cpp"
    )
endif()

set(QHENKIX_PRIVATE_HEADERS
    "${QHENKIX_DIR}/graphics/d3d12/dxc_com_ptr.h"
    "${QHENKIX_DIR}/graphics/d3d12/dxc_include_handler.h"
    "${QHENKIX_DIR}/graphics/d3d12/dxc_shader_compiler.h"

    "${QHENKIX_DIR}/utility/d3d_reflection_util.h"

    "${QHENKIX_DIR}/graphics/vulkan/vulkan_context.h"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_buffer.h"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_descriptor_heap.h"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_command_pool.h"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_texture.h"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_root_signature.h"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_pipeline.h"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_command_list.h"
    "${QHENKIX_DIR}/graphics/vulkan/vulkan_macros.h"
    "${QHENKIX_DIR}/utility/vulkan_util.h"

    "${QHENKIX_DIR}/graphics/shared/descriptor_flush.h"
)

if(WIN32)
    list(APPEND QHENKIX_PRIVATE_HEADERS
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_context.h"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_layout_assembler.h"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_multithread.h"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_pipeline.h"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_texture.h"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_descriptor_heap.h"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_sampler_heap.h"
        "${QHENKIX_DIR}/graphics/d3d11/d3d11_srv_uav_heap.h"
        "${QHENKIX_DIR}/graphics/d3d11/fxc_include_handler.h"
        "${QHENKIX_DIR}/graphics/d3d11/fxc_shader_compiler.h"

        "${QHENKIX_DIR}/graphics/d3d12/d3d12_context.h"
        "${QHENKIX_DIR}/graphics/d3d12/d3d12_descriptor_heap.h"
        "${QHENKIX_DIR}/graphics/d3d12/d3d12_fence.h"
        "${QHENKIX_DIR}/graphics/d3d12/d3d12_pipeline.h"
        "${QHENKIX_DIR}/graphics/d3d12/d3d12_texture.h"
        "${QHENKIX_DIR}/graphics/d3d12/d3d12_command_list.h"
        "${QHENKIX_DIR}/utility/d3d_util.h"
    )
endif()

set(QHENKIX_PUBLIC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/qhenki")

set(QHENKIX_PUBLIC_HEADERS
    "${QHENKIX_PUBLIC_DIR}/application.h"
    "${QHENKIX_PUBLIC_DIR}/camera.h"
    "${QHENKIX_PUBLIC_DIR}/display_window.h"
    "${QHENKIX_PUBLIC_DIR}/orthographic_camera_component.h"
    "${QHENKIX_PUBLIC_DIR}/perspective_camera_component.h"

    "${QHENKIX_PUBLIC_DIR}/2d/spritebatch.h"

    "${QHENKIX_PUBLIC_DIR}/utility/file_util.h"
    "${QHENKIX_PUBLIC_DIR}/utility/gfx_util.h"
    "${QHENKIX_PUBLIC_DIR}/utility/general_util.h"
    "${QHENKIX_PUBLIC_DIR}/utility/directxmath_compat.h"
    "${QHENKIX_PUBLIC_DIR}/utility/math_util.h"
    "${QHENKIX_PUBLIC_DIR}/utility/shader_model_util.h"
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
    "${QHENKIX_PUBLIC_DIR}/RHI/descriptor.h"
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
    "${QHENKIX_PUBLIC_DIR}/RHI/viewport.h"

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

qhenkix_add_dependency_subdirectory(external/DirectXMath ${CMAKE_BINARY_DIR}/DirectXMath)
if(TARGET DirectXMath AND NOT TARGET Microsoft::DirectXMath)
    add_library(Microsoft::DirectXMath ALIAS DirectXMath)
endif()
if(TARGET DirectXMath)
    target_compile_definitions(DirectXMath INTERFACE _XM_NO_XMVECTOR_OVERLOADS_)
    # For sal.h at external/
    if(NOT WIN32)
        target_include_directories(DirectXMath
            INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
        )
    endif()
endif()
set(directxmath_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE PATH "" FORCE)

set(DXHEADERS_INSTALL ON CACHE BOOL "" FORCE)
set(DXHEADERS_BUILD_TEST OFF CACHE BOOL "" FORCE)
qhenkix_add_dependency_subdirectory(external/DirectX-Headers ${CMAKE_BINARY_DIR}/DirectX-Headers)
set(directx-headers_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE PATH "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
qhenkix_add_subdirectory_with_build_shared_libs(external/DirectXTex ${CMAKE_BINARY_DIR}/DirectXTex OFF)
target_link_libraries(${PROJECT_NAME} PUBLIC DirectXTex)
target_link_libraries(${PROJECT_NAME} PUBLIC DirectX-Guids)
target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectXMath)

qhenkix_add_dependency_subdirectory(external/SDL ${CMAKE_BINARY_DIR}/SDL)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3 ${QHENKIX_SYSTEM_LIBS})

set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(VULKAN_HEADERS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
qhenkix_add_dependency_subdirectory(external/Vulkan-Headers ${CMAKE_BINARY_DIR}/Vulkan-Headers)

set(VULKAN_HEADERS_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Headers" CACHE PATH "" FORCE)
set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)
set(VOLK_HEADERS_ONLY OFF CACHE BOOL "" FORCE)
set(VOLK_PULL_IN_VULKAN ON CACHE BOOL "" FORCE)
qhenkix_add_dependency_subdirectory(external/volk ${CMAKE_BINARY_DIR}/volk)

target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)

set(VK_BOOTSTRAP_TEST OFF CACHE BOOL "" FORCE)
set(VK_BOOTSTRAP_INSTALL OFF CACHE BOOL "" FORCE)
qhenkix_add_dependency_subdirectory(external/vk-bootstrap ${CMAKE_BINARY_DIR}/vk-bootstrap)
target_link_libraries(${PROJECT_NAME} PUBLIC vk-bootstrap::vk-bootstrap)

set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "" FORCE)
target_compile_definitions(${PROJECT_NAME} PRIVATE SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
qhenkix_add_dependency_subdirectory(external/SPIRV-Cross ${CMAKE_BINARY_DIR}/spirv-cross)
target_link_libraries(${PROJECT_NAME} PUBLIC spirv-cross-core spirv-cross-glsl)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX UNICODE _UNICODE)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /MP)
    endif()
endif()

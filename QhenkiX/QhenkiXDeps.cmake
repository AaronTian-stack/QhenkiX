# Defines the include directories and link libraries for QhenkiX
# Meant to be reused for both building from source and using as prebuilt library

if(NOT QHENKIX_SRC_DIR)
    set(QHENKIX_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()

set(QHENKIX_INCLUDE_DIRS
    "${QHENKIX_SRC_DIR}/include"
    "${QHENKIX_SRC_DIR}/external/imgui"
    "${QHENKIX_SRC_DIR}/external/imgui/backends"
    "${QHENKIX_SRC_DIR}/external"
    "${QHENKIX_SRC_DIR}/external/SPIRV-Cross/include"
    "${QHENKIX_SRC_DIR}/external/DirectXTex"
    "${QHENKIX_SRC_DIR}/external/utf8"
    "${QHENKIX_SRC_DIR}/external/D3D12MemAlloc"
    "${QHENKIX_SRC_DIR}/external/DirectX-Headers/include"
    "${QHENKIX_SRC_DIR}/external/Vulkan-Headers/include"
    "${QHENKIX_SRC_DIR}/external/volk"
    "${QHENKIX_SRC_DIR}/external/vk-bootstrap/src"
    "${QHENKIX_SRC_DIR}/external/VulkanMemAlloc"
    "${QHENKIX_SRC_DIR}/external/concurrent_queue-1.0.4"
)

if(WIN32)
    set(QHENKIX_SYSTEM_LIBS
        d3d11
        d3d12
        dxgi
        dxguid
        d3dcompiler
        winmm
    )

    set(QHENKIX_DXC_ROOT "${QHENKIX_SRC_DIR}/external/DirectXShaderCompiler")
    set(QHENKIX_DXC_INCLUDE_DIR "${QHENKIX_DXC_ROOT}/dxc_2026_02_20/inc")
    list(APPEND QHENKIX_INCLUDE_DIRS "${QHENKIX_DXC_INCLUDE_DIR}")

    # Choose the appropriate import library directory based on pointer size
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(QHENKIX_DXC_LIB_DIR "${QHENKIX_DXC_ROOT}/dxc_2026_02_20/lib/x64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(QHENKIX_DXC_LIB_DIR "${QHENKIX_DXC_ROOT}/dxc_2026_02_20/lib/x86")
    else()
        message(WARNING "QhenkiXDeps: Unknown pointer size; defaulting DXC library directory to x64")
        set(QHENKIX_DXC_LIB_DIR "${QHENKIX_DXC_ROOT}/dxc_2026_02_20/lib/x64")
    endif()

    list(APPEND QHENKIX_SYSTEM_LIBS
        "${QHENKIX_DXC_LIB_DIR}/dxcompiler.lib"
        "${QHENKIX_DXC_LIB_DIR}/dxil.lib"
    )
else()
    set(QHENKIX_SYSTEM_LIBS "")

    if(UNIX)
        set(QHENKIX_DXC_ROOT "${QHENKIX_SRC_DIR}/external/DirectXShaderCompiler")
        set(QHENKIX_DXC_INCLUDE_DIR "${QHENKIX_DXC_ROOT}/linux_dxc_2026_02_20.x86_64/include")
        list(APPEND QHENKIX_INCLUDE_DIRS "${QHENKIX_DXC_INCLUDE_DIR}")

        list(APPEND QHENKIX_SYSTEM_LIBS
            "${QHENKIX_DXC_ROOT}/linux_dxc_2026_02_20.x86_64/lib/libdxcompiler.so"
            "${QHENKIX_DXC_ROOT}/linux_dxc_2026_02_20.x86_64/lib/libdxil.so"
        )
    endif()
endif()

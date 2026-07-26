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
    "${QHENKIX_SRC_DIR}/external/DirectXMath/Inc"
    "${QHENKIX_SRC_DIR}/external/DirectXTex"
    "${QHENKIX_SRC_DIR}/external/utf8"
    "${QHENKIX_SRC_DIR}/external/D3D12MemAlloc"
    "${QHENKIX_SRC_DIR}/external/DirectX-Headers/include"
    "${QHENKIX_SRC_DIR}/external/Vulkan-Headers/include"
    "${QHENKIX_SRC_DIR}/external/volk"
    "${QHENKIX_SRC_DIR}/external/vk-bootstrap/src"
    "${QHENKIX_SRC_DIR}/external/VulkanMemAlloc"
    "${QHENKIX_SRC_DIR}/external/concurrent_queue-1.0.4"
    "${QHENKIX_SRC_DIR}/external/slang/include"
    "${QHENKIX_SRC_DIR}/external/slang/external/dxc"
    "${QHENKIX_SRC_DIR}/external/SPIRV-Cross/include"
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

else()
    set(QHENKIX_SYSTEM_LIBS "")
endif()

# Defines the include directories and link libraries for QhenkiX
# Meant to be reused for both building from source and using as prebuilt library

if(NOT QHENKIX_SRC_DIR)
    set(QHENKIX_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()

if(NOT SDL3_VERSION)
    set(SDL3_VERSION "3.2.28")
endif()

set(QHENKIX_INCLUDE_DIRS
    "${QHENKIX_SRC_DIR}/include"
    "${QHENKIX_SRC_DIR}/external/imgui"
    "${QHENKIX_SRC_DIR}/external"
    "${QHENKIX_SRC_DIR}/external/DirectXTex"
    "${QHENKIX_SRC_DIR}/external/utf8"
    "${QHENKIX_SRC_DIR}/external/D3D12MemAlloc"
    "${QHENKIX_SRC_DIR}/external/DirectX-Headers-1.618.2/include"
    "${QHENKIX_SRC_DIR}/external/SDL3-${SDL3_VERSION}/include"
)

if(WIN32)
    set(QHENKIX_DIRECTXTEX_LIB_RELEASE "${QHENKIX_SRC_DIR}/external/DirectXTex/lib/x64/Release/DirectXTex.lib")
    set(QHENKIX_DIRECTXTEX_LIB_DEBUG "${QHENKIX_SRC_DIR}/external/DirectXTex/lib/x64/Debug/DirectXTex.lib")
    set(QHENKIX_SDL3_LIB "${QHENKIX_SRC_DIR}/external/SDL3-${SDL3_VERSION}/lib/x64/SDL3.lib")
    
    set(QHENKIX_SYSTEM_LIBS
        d3d11
        d3d12
        dxgi
        d3dcompiler
        dxcompiler
        winmm
    )
endif()

# Helper function to set up DirectXTex
function(qhenkix_setup_DirectXTex)
    if(NOT TARGET DirectXTex)
        add_library(DirectXTex STATIC IMPORTED)
        set_target_properties(DirectXTex PROPERTIES
            IMPORTED_LOCATION_RELEASE "${QHENKIX_DIRECTXTEX_LIB_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${QHENKIX_DIRECTXTEX_LIB_DEBUG}"
            IMPORTED_LOCATION_MINSIZEREL "${QHENKIX_DIRECTXTEX_LIB_RELEASE}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${QHENKIX_DIRECTXTEX_LIB_RELEASE}"
        )
    endif()
endfunction()

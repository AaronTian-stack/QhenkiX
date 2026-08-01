include_guard(GLOBAL)

set(QHENKIX_SLANG_SOURCE_DIR "${REPO_ROOT}/QhenkiX/external/slang")
if(NOT EXISTS "${QHENKIX_SLANG_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Slang submodule is missing. Run: git submodule update --init --recursive")
endif()

if(WIN32)
    set(QHENKIX_SLANG_BINARIES_ROOT
        "${QHENKIX_SLANG_SOURCE_DIR}/external/slang-binaries/bin")
    if(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$"
       OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
        set(QHENKIX_SLANG_BINARIES_DIR "${QHENKIX_SLANG_BINARIES_ROOT}/windows-aarch64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(QHENKIX_SLANG_BINARIES_DIR "${QHENKIX_SLANG_BINARIES_ROOT}/windows-x64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(QHENKIX_SLANG_BINARIES_DIR "${QHENKIX_SLANG_BINARIES_ROOT}/windows-x86")
    else()
        message(FATAL_ERROR "Slang: Unsupported Windows target architecture")
    endif()
endif()

# QhenkiX only needs the compiler library. Disable everything else
set(QHENKIX_SLANG_OPTIONS
    "-DSLANG_ENABLE_CUDA=OFF"
    "-DSLANG_ENABLE_OPTIX=OFF"
    "-DSLANG_ENABLE_NVAPI=OFF"
    "-DSLANG_ENABLE_AFTERMATH=OFF"
    "-DSLANG_ENABLE_GFX=OFF"
    "-DSLANG_ENABLE_SLANG_RHI=OFF"
    "-DSLANG_ENABLE_SLANGD=OFF"
    "-DSLANG_ENABLE_SLANGC=OFF"
    "-DSLANG_ENABLE_SLANGI=OFF"
    "-DSLANG_ENABLE_SLANGRT=OFF"
    "-DSLANG_ENABLE_SLANG_GLSLANG=ON"
    "-DSLANG_ENABLE_TESTS=OFF"
    "-DSLANG_ENABLE_EXAMPLES=OFF"
    "-DSLANG_ENABLE_REPLAYER=OFF"
    "-DSLANG_ENABLE_PREBUILT_BINARIES=ON"
    "-DSLANG_EXCLUDE_DAWN=ON"
    "-DSLANG_EXCLUDE_TINT=ON"
    "-DSLANG_SLANG_LLVM_FLAVOR=DISABLE"
    "-DSLANG_LIB_TYPE=SHARED"
)

if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    # Configure Slang as a nested build and expose only an imported library
    set(QHENKIX_SLANG_BINARY_DIR "${CMAKE_BINARY_DIR}/_slang")
    set(QHENKIX_SLANG_CONFIGURE_COMMAND
        "${CMAKE_COMMAND}"
        -S "${QHENKIX_SLANG_SOURCE_DIR}"
        -B "${QHENKIX_SLANG_BINARY_DIR}"
        -G "${CMAKE_GENERATOR}"
        "-DCMAKE_POLICY_DEFAULT_CMP0141=NEW"
        ${QHENKIX_SLANG_OPTIONS}
    )
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND QHENKIX_SLANG_CONFIGURE_COMMAND -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
        list(APPEND QHENKIX_SLANG_CONFIGURE_COMMAND -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()

    execute_process(
        COMMAND ${QHENKIX_SLANG_CONFIGURE_COMMAND}
        RESULT_VARIABLE QHENKIX_SLANG_CONFIGURE_RESULT
        COMMAND_ECHO STDOUT
    )
    if(NOT QHENKIX_SLANG_CONFIGURE_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to configure the nested Slang build (${QHENKIX_SLANG_CONFIGURE_RESULT})")
    endif()

    add_library(qhenkix_slang SHARED IMPORTED GLOBAL)
    set_target_properties(qhenkix_slang PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS SLANG_DYNAMIC
        INTERFACE_INCLUDE_DIRECTORIES "${QHENKIX_SLANG_SOURCE_DIR}/include"
    )
    foreach(QHENKIX_SLANG_CONFIG Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${QHENKIX_SLANG_CONFIG}" QHENKIX_SLANG_CONFIG_UPPER)
        set_property(TARGET qhenkix_slang PROPERTY
            "IMPORTED_IMPLIB_${QHENKIX_SLANG_CONFIG_UPPER}"
            "${QHENKIX_SLANG_BINARY_DIR}/${QHENKIX_SLANG_CONFIG}/lib/slang-compiler.lib")
        set_property(TARGET qhenkix_slang PROPERTY
            "IMPORTED_LOCATION_${QHENKIX_SLANG_CONFIG_UPPER}"
            "${QHENKIX_SLANG_BINARY_DIR}/${QHENKIX_SLANG_CONFIG}/bin/slang-compiler.dll")
    endforeach()

    add_custom_target(qhenkix_slang_build
        COMMAND "${CMAKE_COMMAND}" --build "${QHENKIX_SLANG_BINARY_DIR}"
            --config $<CONFIG>
            --target slang slang-glslang
            --parallel
        COMMENT "Building the nested Slang compiler dependency"
        VERBATIM
    )
    set_property(TARGET qhenkix_slang_build PROPERTY FOLDER "_Dependencies")
    add_dependencies(qhenkix_slang qhenkix_slang_build)

    set(QHENKIX_SLANG_LINK_TARGET qhenkix_slang)
    set(QHENKIX_SLANG_GLSLANG_FILE
        "${QHENKIX_SLANG_BINARY_DIR}/$<CONFIG>/bin/slang-glslang.dll")
else()
    # Slang otherwise adds its bundled Vulkan-Headers copy to the shared build
    # graph. Add our pinned copy first so both projects reuse one target.
    if(NOT TARGET Vulkan::Headers)
        set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
        set(VULKAN_HEADERS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        qhenkix_add_dependency_subdirectory(
            "${REPO_ROOT}/QhenkiX/external/Vulkan-Headers"
            "${CMAKE_BINARY_DIR}/Vulkan-Headers"
        )
    endif()

    set(SLANG_ENABLE_CUDA OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_OPTIX OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_NVAPI OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_AFTERMATH OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_GFX OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_SLANGC OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_SLANGI OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_SLANG_GLSLANG ON CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_REPLAYER OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_PREBUILT_BINARIES ON CACHE BOOL "" FORCE)
    set(SLANG_EXCLUDE_DAWN ON CACHE BOOL "" FORCE)
    set(SLANG_EXCLUDE_TINT ON CACHE BOOL "" FORCE)
    set(SLANG_SLANG_LLVM_FLAVOR DISABLE CACHE STRING "" FORCE)
    set(SLANG_LIB_TYPE SHARED CACHE STRING "" FORCE)

    add_subdirectory(
        "${QHENKIX_SLANG_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}/external/slang"
        EXCLUDE_FROM_ALL
    )
    set(QHENKIX_SLANG_LINK_TARGET slang)
    set(QHENKIX_SLANG_GLSLANG_FILE "$<TARGET_FILE:slang-glslang>")
endif()

function(qhenkix_stage_dxc_runtime TARGET_NAME)
    if(NOT TARGET "${TARGET_NAME}")
        message(FATAL_ERROR
            "qhenkix_stage_dxc_runtime: '${TARGET_NAME}' is not a CMake target")
    endif()

    if(WIN32)
        set(_qhenkix_dxc_runtime_files
            "${QHENKIX_SLANG_BINARIES_DIR}/dxcompiler.dll"
            "${QHENKIX_SLANG_BINARIES_DIR}/dxil.dll"
        )
        foreach(_qhenkix_dxc_runtime_file IN LISTS _qhenkix_dxc_runtime_files)
            if(NOT EXISTS "${_qhenkix_dxc_runtime_file}")
                message(FATAL_ERROR
                    "Slang's pinned DXC runtime is missing: "
                    "${_qhenkix_dxc_runtime_file}")
            endif()
        endforeach()

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_qhenkix_dxc_runtime_files}
                $<TARGET_FILE_DIR:${TARGET_NAME}>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying Slang's pinned DXC runtime for ${TARGET_NAME}"
        )
    endif()
endfunction()

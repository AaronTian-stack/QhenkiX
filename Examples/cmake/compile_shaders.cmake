option(EXAMPLES_FORCE_SHADER_RECOMPILE "Force recompilation of all shaders" OFF)

function(add_shader_targets TARGET_NAME CONFIG_PATH)
    set(options)
    set(oneValueArgs DX11_SM DX12_SM)
    set(multiValueArgs)
    cmake_parse_arguments(SXC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Defaults
    if(NOT SXC_DX11_SM)
        set(SXC_DX11_SM "5_0")
    endif()
    if(NOT SXC_DX12_SM)
        set(SXC_DX12_SM "6_6")
    endif()

    set(COMPILED_SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/compiled-shaders")
    file(MAKE_DIRECTORY "${COMPILED_SHADERS_DIR}/dx11")
    file(MAKE_DIRECTORY "${COMPILED_SHADERS_DIR}/dx12")

    add_custom_target(${TARGET_NAME}_CompileShaders_DX11 ALL
        COMMAND $<TARGET_FILE:SXC>
            -c "${CONFIG_PATH}"
            -sm ${SXC_DX11_SM}
            -out "${COMPILED_SHADERS_DIR}/dx11"
            -i "${CMAKE_CURRENT_SOURCE_DIR}"
            -i "${CMAKE_SOURCE_DIR}/QhenkiX/include"
            -g DX11=1
            $<$<BOOL:${EXAMPLES_FORCE_SHADER_RECOMPILE}>:-f>
        DEPENDS SXC
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling shaders for ${TARGET_NAME} (DX11)..."
    )

    add_custom_target(${TARGET_NAME}_CompileShaders_DX12 ALL
        COMMAND $<TARGET_FILE:SXC>
            -c "${CONFIG_PATH}"
            -sm ${SXC_DX12_SM}
            -out "${COMPILED_SHADERS_DIR}/dx12"
            -i "${CMAKE_CURRENT_SOURCE_DIR}"
            -i "${CMAKE_SOURCE_DIR}/QhenkiX/include"
            -g DX12=1
            $<$<BOOL:${EXAMPLES_FORCE_SHADER_RECOMPILE}>:-f>
        DEPENDS SXC
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling shaders for ${TARGET_NAME} (DX12)..."
    )

    add_custom_target(${TARGET_NAME}_CompileShaders ALL
        DEPENDS ${TARGET_NAME}_CompileShaders_DX11 ${TARGET_NAME}_CompileShaders_DX12
    )

    set_target_properties(${TARGET_NAME}_CompileShaders_DX11 PROPERTIES FOLDER "Examples/Shaders")
    set_target_properties(${TARGET_NAME}_CompileShaders_DX12 PROPERTIES FOLDER "Examples/Shaders")
    set_target_properties(${TARGET_NAME}_CompileShaders PROPERTIES FOLDER "Examples/Shaders")

    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_CompileShaders)
endfunction()

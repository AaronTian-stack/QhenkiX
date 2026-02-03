cmake_minimum_required(VERSION 3.18)
project(gltfViewer LANGUAGES CXX)

include("${REPO_ROOT}/Examples/cmake/compile_shaders.cmake")

file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/base-shaders/*.hlsl")
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("base-shaders" FILES ${SHADER_FILES})

set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/gltfViewer.config")
set_source_files_properties(${CONFIG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("config" FILES ${CONFIG_FILE})

add_executable(gltfViewer
    main.cpp
    gltf_loader.cpp
    gltf_loader.h
    gltf_model.h
    gltf_viewerapp.cpp
    gltf_viewerapp.h
    scene.h
    ${CONFIG_FILE}
    ${SHADER_FILES}
)

add_shader_targets(gltfViewer "${CMAKE_CURRENT_SOURCE_DIR}/gltfViewer.config")

target_compile_features(gltfViewer PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(gltfViewer PRIVATE /MP)
endif()

# TODO: remove once public headers stop including Windows/D3D types
target_compile_definitions(gltfViewer PRIVATE NOMINMAX)

target_include_directories(gltfViewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/QhenkiX/include
)

target_link_libraries(gltfViewer PRIVATE QhenkiX)

# Set working directory for Visual Studio debugging
set_target_properties(gltfViewer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    FOLDER "Examples"
)

add_custom_command(TARGET gltfViewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${REPO_ROOT}/QhenkiX/external/SDL3-${SDL3_VERSION}/lib/x64/SDL3.dll"
    $<TARGET_FILE_DIR:gltfViewer>
    COMMENT "Copying SDL3.dll to output directory"
)

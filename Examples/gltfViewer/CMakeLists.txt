cmake_minimum_required(VERSION 3.18)
project(gltfViewer LANGUAGES CXX)

include("${REPO_ROOT}/Examples/cmake/CompileShaders.cmake")
include("${REPO_ROOT}/Examples/cmake/ExampleTarget.cmake")

file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/base-shaders/*.hlsl")
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("base-shaders" FILES ${SHADER_FILES})

set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/gltfViewer.config")
set_source_files_properties(${CONFIG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("config" FILES ${CONFIG_FILE})

add_executable(gltfViewer
    main.cpp
    gltf_loader.cpp
    ${REPO_ROOT}/Examples/common/cli_args.cpp
    ${REPO_ROOT}/Examples/common/shader_loader.cpp
    gltf_loader.h
    gltf_model.h
    gltf_viewer_app.cpp
    gltf_viewer_app.h
    scene.h
    scene.cpp
    shared_structs.h
    ${CONFIG_FILE}
    ${SHADER_FILES}
)

add_shader_targets(gltfViewer "${CMAKE_CURRENT_SOURCE_DIR}/gltfViewer.config")

target_compile_features(gltfViewer PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(gltfViewer PRIVATE /MP)
endif()

target_include_directories(gltfViewer PRIVATE
    ${REPO_ROOT}/Examples/include
    ${REPO_ROOT}/QhenkiX/include
)

target_link_libraries(gltfViewer PRIVATE QhenkiX)

qhenkix_configure_example_target(gltfViewer)

add_custom_command(TARGET gltfViewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:gltfViewer>
    COMMENT "Copying SDL3.dll to output directory"
)

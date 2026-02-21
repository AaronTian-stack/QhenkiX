cmake_minimum_required(VERSION 3.18)
project(RetroExample LANGUAGES CXX)

include("${REPO_ROOT}/Examples/cmake/compile_shaders.cmake")

file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/base-shaders/*.hlsl")
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("base-shaders" FILES ${SHADER_FILES})

set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/RetroExample.config")
set_source_files_properties(${CONFIG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("config" FILES ${CONFIG_FILE})

file(GLOB ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
set_source_files_properties(${ASSET_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("assets" FILES ${ASSET_FILES})

add_executable(RetroExample
    main.cpp
    retro_example_app.cpp
    retro_example_app.h
    scene.h
    scene.cpp
    shared_structs.h
    ${CONFIG_FILE}
    ${SHADER_FILES}
    ${ASSET_FILES}
)

add_shader_targets(RetroExample "${CMAKE_CURRENT_SOURCE_DIR}/RetroExample.config")

target_compile_features(RetroExample PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(RetroExample PRIVATE /MP)
endif()

# TODO: remove once public headers stop including Windows/D3D types
target_compile_definitions(RetroExample PRIVATE NOMINMAX)

target_include_directories(RetroExample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/Examples/include
    ${REPO_ROOT}/QhenkiX/include
)

target_link_libraries(RetroExample PRIVATE QhenkiX)

# Set working directory for Visual Studio debugging
set_target_properties(RetroExample PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    FOLDER "Examples"
)

add_custom_command(TARGET RetroExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:RetroExample>
    COMMENT "Copying SDL3.dll to output directory"
)

cmake_minimum_required(VERSION 3.18)
project(ImGuiExample LANGUAGES CXX)

include("${REPO_ROOT}/Examples/cmake/CompileShaders.cmake")

file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/base-shaders/*.hlsl")
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("base-shaders" FILES ${SHADER_FILES})

set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ImGuiExample.config")
set_source_files_properties(${CONFIG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("config" FILES ${CONFIG_FILE})

add_executable(ImGuiExample
    main.cpp
    imgui_example_app.cpp
    imgui_example_app.h
    ${CONFIG_FILE}
    ${SHADER_FILES}
)

add_shader_targets(ImGuiExample "${CMAKE_CURRENT_SOURCE_DIR}/ImGuiExample.config")

target_compile_features(ImGuiExample PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(ImGuiExample PRIVATE /MP)
endif()

target_include_directories(ImGuiExample PRIVATE
    ${REPO_ROOT}/Examples/include
    ${REPO_ROOT}/QhenkiX/include
)

target_link_libraries(ImGuiExample PRIVATE QhenkiX)

# Set working directory for Visual Studio debugging
set_target_properties(ImGuiExample PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    FOLDER "Examples"
)

add_custom_command(TARGET ImGuiExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:ImGuiExample>
    COMMENT "Copying SDL3.dll to output directory"
)

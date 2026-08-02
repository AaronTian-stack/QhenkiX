cmake_minimum_required(VERSION 3.18)
project(SimpleExample LANGUAGES CXX)

include("${REPO_ROOT}/Examples/cmake/CompileShaders.cmake")
include("${REPO_ROOT}/Examples/cmake/ExampleTarget.cmake")

file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/base-shaders/*.slang")
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("base-shaders" FILES ${SHADER_FILES})

set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/SimpleExample.config")
set_source_files_properties(${CONFIG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("config" FILES ${CONFIG_FILE})

add_executable(SimpleExample
    main.cpp
    example_app.cpp
    ${REPO_ROOT}/Examples/common/cli_args.cpp
    ${REPO_ROOT}/Examples/common/shader_loader.cpp
    example_app.h
    shared_structs.h
    ${CONFIG_FILE}
    ${SHADER_FILES}
)

add_shader_targets(SimpleExample "${CMAKE_CURRENT_SOURCE_DIR}/SimpleExample.config")

target_compile_features(SimpleExample PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(SimpleExample PRIVATE /MP)
endif()

target_include_directories(SimpleExample PRIVATE
    ${REPO_ROOT}/Examples/include
    ${REPO_ROOT}/QhenkiX/include
)

target_link_libraries(SimpleExample PRIVATE QhenkiX)

qhenkix_configure_example_target(SimpleExample)

add_custom_command(TARGET SimpleExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:SimpleExample>
    COMMENT "Copying SDL3.dll to output directory"
)

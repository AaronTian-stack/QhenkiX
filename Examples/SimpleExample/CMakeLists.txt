cmake_minimum_required(VERSION 3.18)
project(SimpleExample LANGUAGES CXX)

include("${REPO_ROOT}/Examples/cmake/compile_shaders.cmake")

file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/base-shaders/*.hlsl")
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("base-shaders" FILES ${SHADER_FILES})

add_executable(SimpleExample
    main.cpp
    exampleapp.cpp
    exampleapp.h
    ${SHADER_FILES}
)

add_shader_targets(SimpleExample "${CMAKE_CURRENT_SOURCE_DIR}/SimpleExample.config")

target_compile_features(SimpleExample PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(SimpleExample PRIVATE /MP)
endif()

target_include_directories(SimpleExample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/QhenkiX/include
)

target_link_libraries(SimpleExample PRIVATE QhenkiX)

# Set working directory for Visual Studio debugging
set_target_properties(SimpleExample PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    FOLDER "Examples"
)

add_custom_command(TARGET SimpleExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${REPO_ROOT}/QhenkiX/external/SDL3-${SDL3_VERSION}/lib/x64/SDL3.dll"
    $<TARGET_FILE_DIR:SimpleExample>
    COMMENT "Copying SDL3.dll to output directory"
)

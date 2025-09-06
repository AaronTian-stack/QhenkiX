cmake_minimum_required(VERSION 3.18)
project(SimpleExample LANGUAGES CXX)

add_executable(SimpleExample
    main.cpp
    exampleapp.cpp
    exampleapp.h
)

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
)

add_custom_command(TARGET SimpleExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${REPO_ROOT}/QhenkiX/SDL3-3.2.4/lib/x64/SDL3.dll"
    $<TARGET_FILE_DIR:SimpleExample>
    COMMENT "Copying SDL3.dll to output directory"
)

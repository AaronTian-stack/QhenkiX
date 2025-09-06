cmake_minimum_required(VERSION 3.18)
project(ImGuiExample LANGUAGES CXX)

add_executable(ImGuiExample
    main.cpp
    imgui_exampleapp.cpp
    imgui_exampleapp.h
)

target_compile_features(ImGuiExample PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(ImGuiExample PRIVATE /MP)
endif()

target_include_directories(ImGuiExample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/QhenkiX/include
)

target_link_libraries(ImGuiExample PRIVATE QhenkiX)

# Set working directory for Visual Studio debugging
set_target_properties(ImGuiExample PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_command(TARGET ImGuiExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${REPO_ROOT}/QhenkiX/SDL3-3.2.4/lib/x64/SDL3.dll"
    $<TARGET_FILE_DIR:ImGuiExample>
    COMMENT "Copying SDL3.dll to output directory"
)

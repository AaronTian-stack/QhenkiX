cmake_minimum_required(VERSION 3.18)
project(ImGuiExample LANGUAGES CXX)

add_executable(ImGuiExample
    main.cpp
    imgui_exampleapp.cpp
    imgui_exampleapp.h
)

target_compile_features(ImGuiExample PRIVATE cxx_std_20)

target_include_directories(ImGuiExample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/QhenkiX/include
    ${REPO_ROOT}/QhenkiX/qhenkiX
)

target_link_libraries(ImGuiExample PRIVATE QhenkiX)

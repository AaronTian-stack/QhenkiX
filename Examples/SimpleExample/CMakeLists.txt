cmake_minimum_required(VERSION 3.18)
project(SimpleExample LANGUAGES CXX)

add_executable(SimpleExample
    main.cpp
    exampleapp.cpp
    exampleapp.h
)

target_compile_features(SimpleExample PRIVATE cxx_std_20)

target_include_directories(SimpleExample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/QhenkiX/include
    ${REPO_ROOT}/QhenkiX/qhenkiX
)

target_link_libraries(SimpleExample PRIVATE QhenkiX)

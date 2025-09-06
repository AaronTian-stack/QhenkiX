cmake_minimum_required(VERSION 3.18)
project(SXC LANGUAGES CXX)

set(SXC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler_job.cpp"
)

set(SXC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler_job.h"
)

add_executable(SXC ${SXC_SOURCES} ${SXC_HEADERS})

# Require C++20
target_compile_features(SXC PRIVATE cxx_std_20)

target_include_directories(SXC PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/argparse"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/oneapi"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/tbb"
    "$ENV{RepoRoot}QhenkiX/include"
    "$ENV{RepoRoot}QhenkiX/qhenkiX"
)

target_link_libraries(SXC PRIVATE QhenkiX)

target_link_directories(SXC PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_link_libraries(SXC PRIVATE tbb12.lib)

cmake_minimum_required(VERSION 3.18)
project(SXC LANGUAGES CXX)

set(SXC_TBB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/tbb")
if(NOT EXISTS "${SXC_TBB_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "SXC requires oneTBB source at ${SXC_TBB_SOURCE_DIR}. "
        "Initialize submodules with: git submodule update --init --recursive"
    )
endif()

set(TBB_TEST OFF CACHE BOOL "" FORCE)
set(TBB_INSTALL OFF CACHE BOOL "" FORCE)
set(TBB_VERIFY_DEPENDENCY_SIGNATURE ON CACHE BOOL "" FORCE)

set(_sxc_prev_build_shared_libs "")
if(DEFINED BUILD_SHARED_LIBS)
    set(_sxc_prev_build_shared_libs "${BUILD_SHARED_LIBS}")
endif()
set(BUILD_SHARED_LIBS ON)
add_subdirectory(${SXC_TBB_SOURCE_DIR} ${CMAKE_BINARY_DIR}/oneTBB EXCLUDE_FROM_ALL)
if(_sxc_prev_build_shared_libs STREQUAL "")
    unset(BUILD_SHARED_LIBS)
else()
    set(BUILD_SHARED_LIBS "${_sxc_prev_build_shared_libs}")
endif()

set(SXC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler_job.cpp"
)

set(SXC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler_job.h"
)

add_executable(SXC ${SXC_SOURCES} ${SXC_HEADERS})

target_compile_features(SXC PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(SXC PRIVATE /MP /EHsc)
else()
    target_compile_options(SXC PRIVATE -fexceptions)
endif()

target_include_directories(SXC PRIVATE
    "${REPO_ROOT}/QhenkiX/include"
    "${REPO_ROOT}/QhenkiX/qhenki"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/"
)

target_link_libraries(SXC PRIVATE QhenkiX)
target_link_libraries(SXC PRIVATE TBB::tbb)

if(WIN32)
    add_custom_command(TARGET SXC POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:SXC>
            $<TARGET_FILE_DIR:SXC>
        COMMAND_EXPAND_LISTS
    )
endif()

cmake_minimum_required(VERSION 3.18)
project(gltfViewer LANGUAGES CXX)

add_executable(gltfViewer
    main.cpp
    gltf_loader.cpp
    gltf_loader.h
    gltf_model.h
    gltf_viewerapp.cpp
    gltf_viewerapp.h
)

target_compile_features(gltfViewer PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(gltfViewer PRIVATE /MP)
endif()

target_include_directories(gltfViewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/QhenkiX/include
)

target_link_libraries(gltfViewer PRIVATE QhenkiX)

add_custom_command(TARGET gltfViewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${REPO_ROOT}/QhenkiX/SDL3-3.2.4/lib/x64/SDL3.dll"
    $<TARGET_FILE_DIR:gltfViewer>
    COMMENT "Copying SDL3.dll to output directory"
)

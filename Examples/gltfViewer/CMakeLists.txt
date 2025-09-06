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

target_include_directories(gltfViewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/QhenkiX/include
    ${REPO_ROOT}/QhenkiX/qhenkiX
)

target_link_libraries(gltfViewer PRIVATE QhenkiX)

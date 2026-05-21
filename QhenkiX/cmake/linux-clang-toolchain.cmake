if(NOT DEFINED QHENKIX_REQUIRED_CLANG_VERSION)
    set(QHENKIX_REQUIRED_CLANG_VERSION "21.1" CACHE STRING "Required Clang major.minor version")
endif()

function(qhenkix_verify_clang_version _compiler _language)
    execute_process(
        COMMAND "${_compiler}" --version
        OUTPUT_VARIABLE _qhenkix_clang_version_output
        ERROR_VARIABLE _qhenkix_clang_version_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Extract major.minor from version
    string(REGEX MATCH "clang version ([0-9]+\\.[0-9]+)\\.[0-9]+" _match "${_qhenkix_clang_version_output}")
    if(NOT _match)
        message(FATAL_ERROR
            "${_language} compiler '${_compiler}' is not clang. Output was:\n${_qhenkix_clang_version_output}"
        )
    endif()

    if(NOT CMAKE_MATCH_1 STREQUAL QHENKIX_REQUIRED_CLANG_VERSION)
        message(FATAL_ERROR
            "${_language} compiler '${_compiler}' reports version ${CMAKE_MATCH_1}, but this project requires clang ${QHENKIX_REQUIRED_CLANG_VERSION}.x"
        )
    endif()
endfunction()

# Prefer name `clang-21` but allow `clang` as fallback
if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(QHENKIX_C_COMPILER
        NAMES clang-21 clang clang-${QHENKIX_REQUIRED_CLANG_VERSION}
        REQUIRED
    )
    set(CMAKE_C_COMPILER "${QHENKIX_C_COMPILER}" CACHE FILEPATH "C compiler" FORCE)
endif()

qhenkix_verify_clang_version("${CMAKE_C_COMPILER}" "C")

if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(QHENKIX_CXX_COMPILER
        NAMES clang++-21 clang++ clang++-${QHENKIX_REQUIRED_CLANG_VERSION}
        REQUIRED
    )
    set(CMAKE_CXX_COMPILER "${QHENKIX_CXX_COMPILER}" CACHE FILEPATH "C++ compiler" FORCE)
endif()

qhenkix_verify_clang_version("${CMAKE_CXX_COMPILER}" "C++")

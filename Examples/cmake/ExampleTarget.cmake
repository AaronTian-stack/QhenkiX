function(qhenkix_configure_example_target TARGET_NAME)
    if(ARGC GREATER_EQUAL 2)
        set(_working_dir "${ARGV1}")
    else()
        set(_working_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    get_filename_component(_working_dir "${_working_dir}" ABSOLUTE)

    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER "Examples"
        VS_DEBUGGER_WORKING_DIRECTORY "${_working_dir}"
        XCODE_SCHEME_WORKING_DIRECTORY "${_working_dir}"
    )

    # DXC is used to read reflection embedded in DXIL
    qhenkix_stage_dxc_runtime(${TARGET_NAME})

    if(NOT MSVC AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
        get_property(_banner_shown GLOBAL PROPERTY QHENKIX_EXAMPLE_WD_BANNER_SHOWN)
        if(NOT _banner_shown)
            message("")
            message("==============================================================================")
            message("IMPORTANT: The examples rely on a specific working directory!")
            message("")
            message("Visual Studio and Xcode set this automatically.")
            message("For other IDEs, set the Working Directory of each example's run configuration to the path shown below.")
            message("==============================================================================")
       
        endif()
    endif()
    message(STATUS "  ${TARGET_NAME}  ->  ${_working_dir}")
endfunction()

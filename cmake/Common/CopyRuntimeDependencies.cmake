# internal functions
function(copy_runtime_dependency_for_config dependency configType copyDir)

    set(dll_path)
    
    # We need to get the path of the DLL to be able to copy the file
    if( TARGET ${dependency} )
        # True if the given name is an existing logical target name created by a call
        # to the add_executable(), add_library(), or add_custom_target()
        # command that has already been invoked
        get_target_property(dll_path ${dependency} "LOCATION_${configType}")
    elseif(TARGET ${dependency})
        message(FATAL_ERROR "Invalid target")
    endif(TARGET ${dependency})
    
    message(STATUS "${dependency} DLL: ${dll_path}")
    
    # We only copy DLLs
    if( "${dll_path}" MATCHES "\.lib$" )
        return()
    elseif( NOT "${dll_path}" MATCHES "\.dll$" )
        message(WARNING "DLL for ${dependency} on path ${dll_path} cannot be copied")
        return()
    endif()
    
    message(STATUS "Copy ${dependency}\n\tfrom ${dll_path}\n\tto ${copyDir}")
    file(COPY ${dll_path} DESTINATION "${copyDir}")

endfunction(copy_runtime_dependency_for_config)


function(copy_runtime_dependency dependency)
    # We need to figure out, if the configuration is multi config like 
    # Debug, Release, ... Id so, the dependencie has to be copied to the
    # individual folder
    get_property( generatorIsMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )
    
    if ( ${generatorIsMultiConfig} )
        message(STATUS "isMultiConfig")
        
        foreach( configType ${CMAKE_CONFIGURATION_TYPES} )
            # Set the current output directory based on our configuration
            set(copyDir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${configType}")
            copy_runtime_dependency_for_config(${dependency} ${configType} ${copyDir})
        endforeach()
    endif( ${generatorIsMultiConfig} )

endfunction(copy_runtime_dependency)


#This is the main function called from the individual project
function(copy_runtime_dependencies)
    message(STATUS "------------------------------------")
    message(STATUS "")
    
    foreach(dependency ${ARGN})
        message(STATUS "Copy dependency: ${dependency}")
        copy_runtime_dependency(${dependency})
    endforeach()

    message(STATUS "")
    message(STATUS "------------------------------------")
endfunction()
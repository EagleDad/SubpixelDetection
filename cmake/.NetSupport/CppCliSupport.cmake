
# C++/CLI only supports C++ 17
set(CMAKE_CXX_STANDARD 17)

######################################################################
#
# add_cpp_cli_library
function(add_cpp_cli_executable)
    
     cmake_parse_arguments(
        pargs 
        ""
        "TARGET;CHARACTER_SET;PRE_COMPILED_SOURCE"
        "SOURCES;HEADERS;RESOURCES;DOT_NET_REFERENCES"
        ${ARGN}
    )
    
    if(pargs_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${pargs_UNPARSED_ARGUMENTS}")
    endif()
    
    if( "TARGET" IN_LIST pargs_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "TARGET: not defined!")
    endif()
    
    source_group("Resource Files"
        FILES
            ${pargs_RESOURCES}
    )
    
    source_group("Source Files"
        FILES
            ${pargs_SOURCES}
    )
    
    source_group("Header Files"
        FILES
            ${pargs_HEADERS}
    )
    
    if( NOT "CHARACTER_SET" IN_LIST pargs_KEYWORDS_MISSING_VALUES)
        add_compile_definitions(
            "-D${pargs_CHARACTER_SET} -D_${pargs_CHARACTER_SET}")
    endif()
    
    add_executable(${pargs_TARGET}
        ${pargs_SOURCES}
        ${pargs_HEADERS}
        ${pargs_RESOURCES}
    )
    
    # clr:pure is deprecated
    # https://devblogs.microsoft.com/cppblog/compiler-switch-deprecationremoval-changes-in-visual-studio-14/
    
    # TODO: Make .NET and CLR ad option
    set_target_properties(${pargs_TARGET} PROPERTIES
        DOTNET_TARGET_FRAMEWORK_VERSION ${DOT_NET_VERSION}
        COMMON_LANGUAGE_RUNTIME ""          # TODO: Deprecated. Change to pure
    )
    
    set_property(
        TARGET
            ${pargs_TARGET} 
            
        PROPERTY VS_DOTNET_REFERENCES
            ${pargs_DOT_NET_REFERENCES}
    )
    
    if( NOT "PRE_COMPILED_SOURCE" IN_LIST pargs_KEYWORDS_MISSING_VALUES)
        # For precompiled header.
        # Set 
        # "Precompiled Header" to "Use (/Yu)"
        # "Precompiled Header File" to "stdafx.h"
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Yu${pargs_PRE_COMPILED_SOURCE}.h /FI${pargs_PRE_COMPILED_SOURCE}.h")
        
        set_source_files_properties(
            ${pargs_PRE_COMPILED_SOURCE}.cpp
            PROPERTIES
            COMPILE_FLAGS "/Yc${pargs_PRE_COMPILED_SOURCE}.h"
        )

    endif()
    
endfunction(add_cpp_cli_executable)
######################################################################
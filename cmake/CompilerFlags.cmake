if(NOT CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    message(FATAL_ERROR "Unsupported compiler")
endif()

# This function is used to set the compiler flags for the whol project or 
# can be used to change them for individual projects if needed.
#
# The following presets are supported:
#   STRICT      - Uses as less as possible disabled warnings
#   THIRD_PARTY - Used, when 3rd party modules are involved

function( set_compiler_flags )

    cmake_parse_arguments(pargs "STRICT;THIRD_PARTY" "" "" ${ARGN})
    if(pargs_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${pargs_UNPARSED_ARGUMENTS}")
    endif()

    set(compiler_flags)

    # Warnings:
    if(NOT pargs_THIRD_PARTY)
        list(APPEND compile_flags
            -Wall       # Enable all warnings
            /WX         # Warnings as errors
            /wd4251     # Disable: class 'type1' needs to have dll-interface to be used by clients of class 'type2'
            /wd4275     # Disable: non - DLL-interface class 'class_1' used as base for DLL-interface class 'class_2'
            /wd4514     # Disable: unreferenced inline function has been removed
            /wd4668     # Disable: is not defined as a preprocessor macro, replacing with '0'
            /wd4710     # Disable: function not inlined
            /wd4711     # Disable: function selected for inline expansion
            /wd5045     # Disable: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
            /wd4820     # Disable: bytes padding added after data member
        )
    else()
        list(APPEND compile_flags
            -W2         # Enable warning level 2
            /WX-        # Warnings not as errors
        )
    endif()

	# TODO: USE CXX_FLAGS
    add_definitions( ${compile_flags} )

    add_compile_definitions(
    _USE_MATH_DEFINES
    __STDC_WANT_SECURE_LIB__)

endfunction()
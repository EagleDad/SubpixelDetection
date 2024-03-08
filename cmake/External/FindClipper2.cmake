# Find Clipper library (http://www.angusj.com/delphi/clipper.php).
# The following variables are set
#
# CLIPPER2_FOUND
# CLIPPER2_INCLUDE_DIRS
# CLIPPER2_LIBRARIES

MESSAGE( STATUS "Searching clipper2 library" )

UNSET( CLIPPER2_FOUND CACHE )
UNSET( CLIPPER2_INCLUDE_DIRS CACHE )
UNSET( CLIPPER2_LIBRARIES CACHE )
UNSET( CLIPPER2_LIBRARIES_RELEASE CACHE )
UNSET( CLIPPER2_LIBRARIES_DEBUG CACHE )

if(CMAKE_BUILD_TYPE MATCHES "(Debug|DEBUG|debug)")
    SET( CLIPPER2_BUILD_TYPE DEBUG )
else()
    SET( CLIPPER2_BUILD_TYPE RELEASE )
endif()

FIND_PATH( __CLIPPER2_INCLUDE_DIRS 
    clipper.h
    PATH_SUFFIXES include/clipper2
    PATHS ${Clipper2_DIR}
    NO_CACHE
)

if ( __CLIPPER2_INCLUDE_DIRS )
    SET( CLIPPER2_INCLUDE_DIRS "${Clipper2_DIR}/include" )
endif()

SET(_deb_postfix "-d")

SET( __CLIPPER_2_LIBS "Clipper2;Clipper2Z" )

foreach( _lib ${__CLIPPER_2_LIBS})

    UNSET( __LIB_RELEASE )
    UNSET( __LIB_DEBUG )

    FIND_LIBRARY( __LIB_RELEASE
        ${_lib}
        PATH_SUFFIXES lib
        PATHS ${Clipper2_DIR}
        NO_CACHE
    )
    
    
    if ( __LIB_RELEASE )
        list( APPEND 
            CLIPPER2_LIBRARIES_RELEASE
            ${__LIB_RELEASE}
        )
    endif()
    
    FIND_LIBRARY( __LIB_DEBUG
        ${_lib}${_deb_postfix}
        PATH_SUFFIXES lib
        PATHS ${Clipper2_DIR}
        NO_CACHE
    )
    
    if ( __LIB_DEBUG )
        list( APPEND 
            CLIPPER2_LIBRARIES_DEBUG
            ${__LIB_DEBUG}
        )
    endif()

endforeach()

if( CLIPPER2_LIBRARIES_${CLIPPER2_BUILD_TYPE})
    SET( CLIPPER2_LIBRARIES "${CLIPPER2_LIBRARIES_${CLIPPER2_BUILD_TYPE}}" )
    MESSAGE(STATUS "1")
else()
    SET( CLIPPER2_LIBRARIES "${CLIPPER2_LIBRARIES_RELEASE}" )
    MESSAGE(STATUS "2")
endif()

MESSAGE(STATUS "CLIPPER2_LIBRARIES: ${CLIPPER2_LIBRARIES}")
MESSAGE(STATUS "CLIPPER2_LIBRARIES_RELEASE: ${CLIPPER2_LIBRARIES_RELEASE}")
MESSAGE(STATUS "CLIPPER2_LIBRARIES_DEBUG: ${CLIPPER2_LIBRARIES_DEBUG}")

SET( CLIPPER2_PATH ${Clipper2_DIR})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Clipper2
    "Clipper2 library cannot be found.  Consider set CLIPPER2_PATH environment variable"
    CLIPPER2_INCLUDE_DIRS
    CLIPPER2_LIBRARIES
)

MARK_AS_ADVANCED(
    CLIPPER_INCLUDE_DIRS
    CLIPPER_LIBRARIES
)

if( CLIPPER2_FOUND )

    foreach( _lib IN ZIP_LISTS __CLIPPER_2_LIBS CLIPPER2_LIBRARIES_DEBUG CLIPPER2_LIBRARIES_RELEASE)
    
        set(LIBRARY_NAME "Clipper2::${_lib_0}")
        get_raw_target_name(${LIBRARY_NAME} LIBRARY_NAME_RAW)
    
        MESSAGE(STATUS "LIBRARY_NAME: ${LIBRARY_NAME}")
        MESSAGE(STATUS "LIBRARY_NAME_RAW: ${LIBRARY_NAME_RAW}")
        
        add_library(${LIBRARY_NAME_RAW} UNKNOWN IMPORTED)
        add_library(${LIBRARY_NAME} ALIAS ${LIBRARY_NAME_RAW})
        
        set_target_properties( ${LIBRARY_NAME_RAW}
            PROPERTIES 
                INTERFACE_INCLUDE_DIRECTORIES ${CLIPPER2_INCLUDE_DIRS}
        )
        
        if( CLIPPER2_LIBRARIES_RELEASE AND CLIPPER2_LIBRARIES_DEBUG )
        
            set_target_properties(${LIBRARY_NAME_RAW} PROPERTIES
                IMPORTED_LOCATION_DEBUG          ${_lib_1}
                IMPORTED_LOCATION_RELWITHDEBINFO ${_lib_2}
                IMPORTED_LOCATION_RELEASE        ${_lib_2}
                IMPORTED_LOCATION_MINSIZEREL     ${_lib_2}
            )
        endif()
        
    endforeach()

endif()


if ( MSVC )
    set(DOT_NET_VERSION "v4.7.2" CACHE STRING "The .NET version to use for the projects")
    set_property(CACHE DOT_NET_VERSION PROPERTY STRINGS v4.8 v4.7.2 v4.6.2 v4.6.1 )

    include(CppCliSupport)
    include(CSharpUtilities)
endif ( MSVC )

######################################################################
#
# load_nuget_package
# net40 net461 net35 net 45 etc need to be a parameter
function(load_nuget_package packageName packageVersion)

    if ( ${packageName} STREQUAL "" OR ${packageVersion} STREQUAL "" )
        message(FATAL_ERROR "Invalid nuget package information for packageName (${packageName}) with packageVersion (${packageVersion})!")
    else()
        message(STATUS "Loading nuget package ${packageName} with version ${packageVersion}")
    endif()
    
    set(NUGET_PKG_FOUND FALSE PARENT_SCOPE)
    set(NUGET_DOTNET_REFERENCE)
    set(CMP_STRING "${packageName} ${packageVersion}")
    set(PACKAGE_DEST "${CMAKE_BINARY_DIR}/packages/${packageName}/${packageVersion}")
    set(OUT_VAL "EMPTY")
    set(RET_VAL)
    
    # Find nuget. It is loated in the repo
    if ( NOT NUGET_EXE )
        message(STATUS "Searching for nuget")
        find_program(NUGET_EXE NAMES nuget nuget
            PATHS "${CMAKE_SOURCE_DIR}/common/.nuget"
        )
    else( NOT NUGET_EXE )
        message(STATUS "nuget already found")
    endif( NOT NUGET_EXE )
    
    if ( NOT NUGET_EXE )
        message(FATAL_ERROR "nuget not found. Consider installation or check path")
    endif()
    
    # Check if package is already installed
    if ( EXISTS ${PACKAGE_DEST}/packages )
        exec_program(${NUGET_EXE}
            ARGS list ${packageName} -AllVersions -Source ${PACKAGE_DEST}
            OUTPUT_VARIABLE OUT_VAL
            RETURN_VALUE RET_VAL
        )
    endif()
    
    message(STATUS "OUT_VAL: ${OUT_VAL}")
    message(STATUS "RET_VAL: ${RET_VAL}")
    
    if ( ${CMP_STRING} STREQUAL ${OUT_VAL} )
        message(STATUS "${CMP_STRING} already installed. Skipping installation")
    else()
        message(STATUS "${CMP_STRING} not installed. Installing package")
        exec_program(${NUGET_EXE}
            ARGS install ${packageName} -Version ${packageVersion} -ExcludeVersion -OutputDirectory ${PACKAGE_DEST}
            OUTPUT_VARIABLE OUT_VAL
            RETURN_VALUE RET_VAL
        )
        
        if (NOT ${RET_VAL} EQUAL 0)
           message(FATAL_ERROR "Unable to install nuget package ${packageVersion} with version ${packageVersion}")
        endif()
        
    endif()
    
    ## OUT VAR
    set(NUGET_DOTNET_REFERENCE "${CMAKE_BINARY_DIR}/packages/${packageName}/${packageVersion}/${packageName}/lib/net45")
    FILE(GLOB OUT "${NUGET_DOTNET_REFERENCE}/*.dll")
    set(NUGET_DOTNET_REFERENCE ${OUT} PARENT_SCOPE)
    #message(STATUS "NUGET_DOTNET_REFERENCE: ${NUGET_DOTNET_REFERENCE}")

    set(NUGET_PKG_FOUND TRUE PARENT_SCOPE)

endfunction(load_nuget_package)
######################################################################

######################################################################
#
# load_nuget_package
function(add_nuget_package)

    message(STATUS "ARGN: ${ARGN}")

     cmake_parse_arguments(
        pargs 
        ""
        "TARGET;PACKAGE;VERSION"
        ""
        ${ARGN}
    )
    
    if(pargs_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${pargs_UNPARSED_ARGUMENTS}")
    endif()
    
    if ( NOT TARGET ${pargs_TARGET} )
         message(FATAL_ERROR "Invalid target")
    endif()

    message(STATUS "Adding nuget package ${pargs_PACKAGE} with version ${pargs_VERSION}")
    load_nuget_package(${pargs_PACKAGE} ${pargs_VERSION})
    
    if ( NOT ${NUGET_PKG_FOUND} )
        message(FATAL_ERROR "nuget package ${packageName} with version ${packageVersion} not foud")
    endif()
    
    get_target_property(
        EXISTING_PROPS
        ${LIBRARY_NAME} 
        VS_DOTNET_REFERENCES
    )
    
    message(STATUS "NUGET_DOTNET_REFERENCE: ${NUGET_DOTNET_REFERENCE}")
    message(STATUS "EXISTING_PROPS: ${EXISTING_PROPS}")
    
    set_property(
        TARGET
            ${pargs_TARGET} 
        PROPERTY VS_DOTNET_REFERENCES
            ${EXISTING_PROPS}
            ${NUGET_DOTNET_REFERENCE}
    )

endfunction(add_nuget_package)
######################################################################

######################################################################
#
# create_default_application_files
function(create_default_application_files)

    # Setup minimum required source files
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/App.xaml")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/App.xaml.in
            ${CMAKE_CURRENT_SOURCE_DIR}/App.xaml
        )
    endif()
    set(WPF_REQIRED_XAML_SOURCE_FILES ${WPF_REQIRED_XAML_SOURCE_FILES} App.xaml)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/App.xaml.cs")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/App.xaml.cs.in
            ${CMAKE_CURRENT_SOURCE_DIR}/App.xaml.cs
        )
    endif()
    set(WPF_REQIRED_XAML_SOURCE_FILES ${WPF_REQIRED_XAML_SOURCE_FILES} App.xaml.cs)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.xaml.cs")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/MainWindow.xaml.cs.in
            ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.xaml.cs
        )
    endif()
    set(WPF_REQIRED_XAML_SOURCE_FILES ${WPF_REQIRED_XAML_SOURCE_FILES} MainWindow.xaml.cs)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.xaml")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/MainWindow.xaml.in
            ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.xaml
        )
    endif()
    set(WPF_REQIRED_XAML_SOURCE_FILES ${WPF_REQIRED_XAML_SOURCE_FILES} MainWindow.xaml)
    
    set(WPF_REQIRED_XAML_SOURCE_FILES ${WPF_REQIRED_XAML_SOURCE_FILES} PARENT_SCOPE)
    
endfunction(create_default_application_files)
######################################################################

######################################################################
#
# create_default_resource_files
function(create_default_resource_files)

    # Setup propertiy files
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Properties/AssemblyInfo.cs")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Properties/AssemblyInfo.cs.in
            ${CMAKE_CURRENT_SOURCE_DIR}/Properties/AssemblyInfo.cs
        )
    endif()
    set(WPF_REQIRED_PROPERTY_FILES ${WPF_REQIRED_PROPERTY_FILES} Properties/AssemblyInfo.cs)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Properties/Resources.Designer.cs")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Properties/Resources.Designer.cs.in
            ${CMAKE_CURRENT_SOURCE_DIR}/Properties/Resources.Designer.cs
        )
    endif()
    set(WPF_REQIRED_PROPERTY_FILES ${WPF_REQIRED_PROPERTY_FILES} Properties/Resources.Designer.cs)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Properties/Resources.resx")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Properties/Resources.resx.in
            ${CMAKE_CURRENT_SOURCE_DIR}/Properties/Resources.resx
        )
    endif()
    set(WPF_REQIRED_PROPERTY_FILES ${WPF_REQIRED_PROPERTY_FILES} Properties/Resources.resx)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Properties/Settings.Designer.cs")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Properties/Settings.Designer.cs.in
            ${CMAKE_CURRENT_SOURCE_DIR}/Properties/Settings.Designer.cs
        )
    endif()
    set(WPF_REQIRED_PROPERTY_FILES ${WPF_REQIRED_PROPERTY_FILES} Properties/Settings.Designer.cs)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Properties/Settings.settings")
        configure_file(
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Properties/Settings.settings.in
            ${CMAKE_CURRENT_SOURCE_DIR}/Properties/Settings.settings
        )
    endif()
    set(WPF_REQIRED_PROPERTY_FILES ${WPF_REQIRED_PROPERTY_FILES} Properties/Settings.settings)
    
    set(WPF_REQIRED_PROPERTY_FILES ${WPF_REQIRED_PROPERTY_FILES} PARENT_SCOPE)
    
endfunction(create_default_resource_files)
######################################################################

######################################################################
#
# add_csharp_library
function(add_csharp_library)

    cmake_parse_arguments(
        pargs 
        "CREATE_DEFAULT_FILES"
        "TARGET"
        "SOURCES;CONFIGURATION;PROPERTIES;RESOURCES;DOT_NET_REFERENCES;FILES"
        ${ARGN}
    )
    
    if(pargs_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${pargs_UNPARSED_ARGUMENTS}")
    endif()
    
    set(WPF_REQIRED_XAML_SOURCE_FILES "")
    set(WPF_REQIRED_CONFIG_FILES "")
    set(WPF_REQIRED_PROPERTY_FILES "")
    
    if( "TARGET" IN_LIST pargs_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "TARGET: not defined!")
    endif()
    
    source_group("Source Files"
        FILES
            ${pargs_SOURCES}
    )
    
    source_group("Resource Files"
        FILES
            ${pargs_RESOURCES}
    )
    
    if ( ${pargs_CREATE_DEFAULT_FILES})
        create_default_resource_files()
    endif()
    
    add_library(${pargs_TARGET} SHARED
        ${pargs_CONFIGURATION}
        ${pargs_SOURCES}
        ${pargs_PROPERTIES}
        ${pargs_RESOURCES}
        ${pargs_FILES}
        ${WPF_REQIRED_PROPERTY_FILES}
    )
    
    csharp_set_designer_cs_properties(
        ${WPF_REQIRED_PROPERTY_FILES}
    )
    
    set_property(
        TARGET
            ${pargs_TARGET}
        
        PROPERTY
            WIN32_EXECUTABLE FALSE
    )
    
    set_property(
        TARGET
            ${pargs_TARGET}
        
        PROPERTY
            VS_DOTNET_TARGET_FRAMEWORK_VERSION "${DOT_NET_VERSION}"
    )
    
    set_property(
        TARGET
            ${pargs_TARGET} 
            
        PROPERTY VS_DOTNET_REFERENCES
            ${pargs_DOT_NET_REFERENCES}
    )
    
    set_property(
        SOURCE 
            ${pargs_RESOURCES}
            
        PROPERTY VS_TOOL_OVERRIDE
            "Resource"
    )

endfunction(add_csharp_library)
######################################################################

######################################################################
#
# add_wpf_executable
function(add_wpf_executable)

    cmake_parse_arguments(
        pargs 
        "CREATE_DEFAULT_FILES"
        "TARGET;APP_CONFIG"
        "SOURCES;XAML_SOURCES;DOT_NET_REFERENCES;RESOURCES"
        ${ARGN}
    )
    
    if(pargs_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${pargs_UNPARSED_ARGUMENTS}")
    endif()
    
    set(WPF_REQIRED_XAML_SOURCE_FILES "")
    set(WPF_REQIRED_CONFIG_FILES "")
    set(WPF_REQIRED_PROPERTY_FILES "")
    
    # WPF Applocations need a App.config file. If this is not passed we have to create from template
    if( NOT "APP_CONFIG" IN_LIST pargs_KEYWORDS_MISSING_VALUES)
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/App.config")
            configure_file(
                ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/App.config.in
                ${CMAKE_CURRENT_SOURCE_DIR}/App.config
            )
        endif()
    endif()
    list(APPEND WPF_REQIRED_CONFIG_FILES App.config)
    
    if ( ${pargs_CREATE_DEFAULT_FILES})
        create_default_application_files()
        create_default_resource_files()
    endif()
    
    set(WPF_REQIRED_XAML_SOURCE_FILES ${WPF_REQIRED_XAML_SOURCE_FILES} ${pargs_XAML_SOURCES})
    
    source_group("Resource Files"
        FILES
            ${pargs_RESOURCES}
    )
    
    add_executable(${pargs_TARGET}

        ${WPF_REQIRED_CONFIG_FILES}

        ${WPF_REQIRED_XAML_SOURCE_FILES}

        ${WPF_REQIRED_PROPERTY_FILES}
    
        ${pargs_SOURCES}
        
        ${pargs_RESOURCES}
    )
    
    csharp_set_designer_cs_properties(
        ${WPF_REQIRED_PROPERTY_FILES}
    )
    
    csharp_set_xaml_cs_properties(
        ${WPF_REQIRED_XAML_SOURCE_FILES}
    )
    
    set_property(
        SOURCE
            App.xaml
        
        PROPERTY
            VS_XAML_TYPE "ApplicationDefinition"
    )
    
    set_property(
        TARGET
            ${pargs_TARGET}
        
        PROPERTY
            VS_DOTNET_TARGET_FRAMEWORK_VERSION "${DOT_NET_VERSION}"
    )
    
    set_property(
        TARGET
            ${pargs_TARGET}
        
        PROPERTY
            WIN32_EXECUTABLE TRUE
    )
    
    set_property(
        TARGET
            ${pargs_TARGET} 
            
        PROPERTY VS_DOTNET_REFERENCES
            ${pargs_DOT_NET_REFERENCES}
    )
    
    set_property(
        SOURCE 
            ${pargs_RESOURCES}
            
        PROPERTY VS_TOOL_OVERRIDE
            "Resource"
    )

endfunction(add_wpf_executable)
######################################################################
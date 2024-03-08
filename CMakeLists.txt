cmake_minimum_required(VERSION 3.11)

# Configure C++
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(COPYRIGHT_YEAR_START 2022)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Checks
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/External
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/.NetSupport
)

# Force all libraries to be build dynamic if ON, also they are set to STATIC
# If we set this flag to on and use gtest with FectContent we have to
# implement the main function for our unit tests by ourself
option(BUILD_SHARED_LIBS            "Build project libraries as shared libraries" OFF)
option(BUILD_TESTING                "Build with tests" OFF)
option(BUILD_PYTHON_BINDINGS        "Build bindings for python" OFF)
option(BUILD_DOC                    "Build documentation" OFF)
option(BUILD_WITH_CLIPPER           "Build some libraries with clipper support" OFF)
option(BUILD_WITH_EIGEN             "Build some libraries with eigen support" OFF)
option(BUILD_WITH_BOOST             "Build some libraries with boost support" OFF)
option(BUILD_WITH_OPENCV            "Build some libraries with OpenCV support" ON)
option(BUILD_WITH_OPENCV_CONTRIB    "Build some libraries with OpenCV contrib support" ON)
option(BUILD_WITH_NLOHMAN_JSON      "Build some libraries with JSON support" OFF)
option(BUILD_WITH_ZIP               "Build some libraries with Zip support" OFF)
option(BUILD_WITH_QT                "Build some libraries with Qt support" OFF)

option(DEBUG_POSTFIX                "Set to ON to force _d postfix for the lib names." OFF)

option(ENABLE_SOLUTION_FOLDERS      "Solution folder in Visual Studio or in other IDEs" ON)

option(STATIC_CODE_ANALYSIS         "Enables static code analysis with clang-tidy and CppCheck, if installed." OFF)

option(COVERAGE_ON                  "Enable code coverage" OFF)

# 3rd party dependencies needs to be build first
option (USE_SUPERBUILD              "Whether or not a superbuild should be invoked" ON)

# Build type
set(BUILD_TYPE "Engineering" CACHE STRING "The build type to use for the projects")
set_property(CACHE BUILD_TYPE PROPERTY STRINGS Internal Official Engineering)

if (USE_SUPERBUILD)
    # execute the superbuild (this script will be invoked again without the
    # USE_SUPERBUILD option this time)
    include(SuperBuild)
    return()                      # stop processing this file further
else(USE_SUPERBUILD)
    include(VersionInfo)
    project(SubpixelDetection VERSION ${PROJECT_VERSION_FULL})
endif(USE_SUPERBUILD)

if(ENABLE_SOLUTION_FOLDERS)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
endif()

# Standard CMake modules
include(FindPackageHandleStandardArgs) 
include(GenerateExportHeader)

# Modules
include(Common)
include(CompilerFlags)

if(DEBUG_POSTFIX)
    # For debug libs and exes, add "-d" postfix
    set(CMAKE_DEBUG_POSTFIX "-d" CACHE STRING "Set debug library postfix")
endif(DEBUG_POSTFIX)

if(BUILD_TESTING)
    # Activate test support
    # Enable integration with CTest
    # The shared libs option needs to be turne off to be alble to use gtest main function 
    # instead of writing every time our own. Maybe for all test executables a own fucntion is required
    include(CTest)
    include(GoogleTestTesting)
    enable_testing()
endif(BUILD_TESTING)

if(BUILD_DOC)
    #include(DoxygenSupport)
    #add_subdirectory(doc)
endif(BUILD_DOC)

# Add clang-format for all C++ projects
# and editorconfig for all .NET projects
add_custom_target(ALL
    SOURCES 
        .clang-format
        .editorconfig
        .clang-tidy
)

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory" FORCE)

message(STATUS "")
message(STATUS "  Install to: " "${CMAKE_INSTALL_PREFIX}")
message(STATUS "-----------------------------------------------------------------")
message(STATUS "")

# Let CMake load the CMakeLists.txt in the subdirectory to load the
# sub-projects. Prefer lower-case names.
#add_subdirectory(libraries)
add_subdirectory(applications)

message(STATUS "Version: ${PROJECT_VERSION_FULL}")

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_FULL})

#Set the name of the generated installation package
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})

#Set the directory path to install to
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages") 

set(CPACK_PACKAGE_VENDOR "bucher emhart glass")

#set(CPACK_NUGET_PACKAGE_LICENSE_FILE_NAME "${CMAKE_SOURCE_DIR}/Doc/License/License.txt")

if(WIN32)
    set(CPACK_NUGET_PACKAGE_TAGS "Native" "native")
    set(CPACK_GENERATOR ZIP NuGet)
else()
    set(CPACK_GENERATOR TGZ)
endif()

set(CPACK_INSTALL_CMAKE_CONFIGURATIONS Debug Release)

include(CPack)


set_compiler_flags(THIRD_PARTY)

# Don't use PROJECT_NAME as target name, it's too tight of a relationship
set(EXECUTABLE_NAME "subPixelEdgeDetection")

find_package(OpenCV  REQUIRED )

message(STATUS "OPENCV_ROOT: ${OPENCV_ROOT}")
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")
message(STATUS "OpenCV_DIR: ${OpenCV_DIR}")

include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable(${EXECUTABLE_NAME}
    main.cpp
    SubPixelDetection.cpp
    SubPixelDetection.h
)

if(ENABLE_SOLUTION_FOLDERS)
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "applications")
endif()

target_include_directories(${EXECUTABLE_NAME}
    INTERFACE
    
    PUBLIC
)

copy_runtime_dependencies(
    ${OpenCV_LIBS}
)

target_link_libraries(${EXECUTABLE_NAME}
    PUBLIC
        ${OpenCV_LIBS}
    PRIVATE
        
    INTERFACE
)

install(TARGETS ${EXECUTABLE_NAME}
    ARCHIVE  DESTINATION "lib"
    LIBRARY  DESTINATION "bin"
    RUNTIME  DESTINATION "bin"
)

install(
    FILES
    DESTINATION
        "bin"
)

install(
    IMPORTED_RUNTIME_ARTIFACTS ${OpenCV_LIBS}
    DESTINATION "bin"
)
ExternalProject_Add(
    pybind11
    PREFIX ${FETCHCONTENT_BASE_DIR}/pybind11
    URL https://github.com/pybind/pybind11/archive/refs/tags/v2.9.2.zip
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/pybind11/pybind11-2.9.2/src"
    BINARY_DIR "${FETCHCONTENT_BASE_DIR}/pybind11/pybind11-2.9.2/build"
    INSTALL_DIR "${FETCHCONTENT_BASE_DIR}/pybind11/pybind11-2.9.2/install"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        -DPYBIND11_TEST=OFF
    #-DPYTHON_LIBRARY=${Python_LIBRARIES}
    #-DPYTHON_INCLUDE_DIR=${Python_INCLUDE_DIRS}
)

list(APPEND DEPENDENCIES pybind11)

set(pybind11_DIR "${FETCHCONTENT_BASE_DIR}/pybind11/pybind11-2.9.2/install/share/cmake/pybind11")

message(STATUS "pybind11_DIR: ${pybind11_DIR}")

list(APPEND EXTRA_CMAKE_ARGS
    -Dpybind11_DIR:string=${pybind11_DIR}
)


ExternalProject_Add(
    eigen
#    GIT_REPOSITORY      "https://gitlab.com/libeigen/eigen.git"
#    GIT_TAG             "3.4.0"
    PREFIX ${FETCHCONTENT_BASE_DIR}/eigen
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/eigen/eigen-3.4.0/src"
    BINARY_DIR "${FETCHCONTENT_BASE_DIR}/eigen/eigen-3.4.0/build"
    INSTALL_DIR "${FETCHCONTENT_BASE_DIR}/eigen/eigen-3.4.0/install"
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=Release
)

list(APPEND DEPENDENCIES eigen)
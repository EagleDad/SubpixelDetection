
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

ExternalProject_Add(
    googletest
    PREFIX ${FETCHCONTENT_BASE_DIR}/googletest
    GIT_REPOSITORY      https://github.com/google/googletest.git
    GIT_TAG             release-1.10.0
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/googletest/googletest-1.10.0/src"
    BINARY_DIR "${FETCHCONTENT_BASE_DIR}/googletest/googletest-1.10.0/build"
    INSTALL_DIR "${FETCHCONTENT_BASE_DIR}/googletest/googletest-1.10.0/install"
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=Release
    -Dgtest_force_shared_crt=ON
)

list(APPEND DEPENDENCIES googletest)

set(GTest_DIR "${FETCHCONTENT_BASE_DIR}/googletest/googletest-1.10.0/install/lib/cmake/GTest")

list(APPEND EXTRA_CMAKE_ARGS
    -DGTest_DIR:PATH=${GTest_DIR}
)






set( BOOST_BOOTSTRAP_CCOMMAND "" )
set( BOOST_B2B_CCOMMAND "" )

if( UNIX )
  set( BOOST_BOOTSTRAP_CCOMMAND ./bootstrap.sh )
  set( BOOST_B2B_CCOMMAND ./b2 )
else()
  if( WIN32 )
    set( BOOST_BOOTSTRAP_CCOMMAND bootstrap.bat )
    set( BOOST_B2B_CCOMMAND ./b2 )
  endif()
endif()

SET(BOOST_CONFIGURE <SOURCE_DIR>/${BOOST_BOOTSTRAP_CCOMMAND} --prefix=<SOURCE_DIR>)
SET(BOOST_INSTALL <SOURCE_DIR>/${BOOST_B2B_CCOMMAND} install --prefix=<BINARY_DIR>)

ExternalProject_Add(
    boost
    URL https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.zip
    PREFIX ${FETCHCONTENT_BASE_DIR}/boost
    BUILD_IN_SOURCE true
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CONFIGURE_COMMAND ${BOOST_CONFIGURE}
    BUILD_COMMAND  ""
    INSTALL_COMMAND ${BOOST_INSTALL}
)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
set(Boost_DIR "${FETCHCONTENT_BASE_DIR}/boost/src")
set(Boost_INCLUDE_DIR "${FETCHCONTENT_BASE_DIR}/boost/src/boost")

list(APPEND DEPENDENCIES boost)

list(APPEND EXTRA_CMAKE_ARGS
   -DBOOST_ROOT:PATH=${Boost_DIR}
   -DBoost_NO_SYSTEM_PATHS:BOOL=ON
)
# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-vp4-update)

pkg_check_modules(PKG_CFG_OSTREE_1 REQUIRED ostree-1)

if(PKG_CFG_OSTREE_1_FOUND)
  message(STATUS "ostree-1 found:")
  message(STATUS "  Version: ${PKG_CFG_OSTREE_1_VERSION}")
  message(STATUS "  Include directories: ${PKG_CFG_OSTREE_1_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${PKG_CFG_OSTREE_1_LIBRARIES}")
  message(STATUS "  Definitions: ${PKG_CFG_OSTREE_1_DEFINITIONS}")
  message(STATUS "  Compile options: ${PKG_CFG_OSTREE_1_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "ostree-1 not found")
endif()

pkg_check_modules(PKG_CFG_AKTUALIZR REQUIRED aktualizr)

if(PKG_CFG_AKTUALIZR_FOUND)
  message(STATUS "aktualizr found:")
  message(STATUS "  Version: ${PKG_CFG_AKTUALIZR_VERSION}")
  message(STATUS "  Include directories: ${PKG_CFG_AKTUALIZR_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${PKG_CFG_AKTUALIZR_LIBRARIES}")
  message(STATUS "  Definitions: ${PKG_CFG_AKTUALIZR_DEFINITIONS}")
  message(STATUS "  Compile options: ${PKG_CFG_AKTUALIZR_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "aktualizr not found")
endif()

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiUpdate.cpp
)

set(${VPE_LIB}_LIBRARIES
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
  ${PKG_CFG_OSTREE_1_LIBRARIES}
  boost_log
  boost_filesystem
  aktualizr_lite
  aktualizr
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ../../../common/platform
  ../../common
)

set(${VPE_LIB}_INCLUDE_DIRECTORIES
  ${PKG_CFG_OSTREE_1_INCLUDE_DIRS}
  ${PKG_CFG_AKTUALIZR_INCLUDE_DIRS}
)

set(${VPE_LIB}_COMPILE_DEFINITIONS
  BOOST_LOG_DYN_LINK
)

set(${VPE_LIB}_COMPILE_OPTIONS
  ${PKG_CFG_OSTREE_1_CFLAGS}
  ${PKG_CFG_AKTUALIZR_CFLAGS}
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

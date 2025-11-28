# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-common-dbus)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  DbusObject.cpp
  DbusConnection.cpp
)

set(${VPE_LIB}_LIBRARIES
  ${PKG_CFG_GLIB_LIBRARIES}
)

set(${VPE_LIB}_COMPILE_OPTIONS
  -Wno-deprecated-declarations
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-common-update)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiUpdateWriter.cpp
  CstiUpdateConnection.cpp
  CstiUpdate.cpp
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ..
)  

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

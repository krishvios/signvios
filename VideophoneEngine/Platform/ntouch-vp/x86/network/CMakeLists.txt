# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-x86-network)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiDHCP.cpp
  CstiNetwork.cpp
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ../../../common/wireless
  ../../../common/network
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

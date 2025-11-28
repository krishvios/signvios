# Define the library name
set(VPE_LIB stiSession)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiSdp.cpp
  stiPayloadTypes.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiLibraries
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

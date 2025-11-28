# Define the library name
set(VPE_LIB stiTextRecord)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiTextRecord.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiDataCommon
)

set(${VPE_LIB}_COMPILE_DEFINITIONS
  stiSILENCE_SUPPRESSION
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})


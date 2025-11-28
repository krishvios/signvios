# Define the library name
set(VPE_LIB stiPlatform-common-fileplay-MPEG4-Toolbox)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  SMMemory.cpp
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

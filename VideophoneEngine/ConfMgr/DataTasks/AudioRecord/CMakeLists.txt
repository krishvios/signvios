# Define the library name
set(VPE_LIB stiAudioRecord)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiAudioRecord.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiLibraries
)

set(${VPE_LIB}_COMPILE_DEFINITIONS
  stiSILENCE_SUPPRESSION
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

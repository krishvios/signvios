# Define the library name
set(VPE_LIB stiAudioPlayback)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiAudioPlayback.cpp
  CstiAudioPlaybackRead.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiLibraries
)

set(${VPE_LIB}_COMPILE_DEFINITIONS
  stiSILENCE_SUPPRESSION
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

# Define the library name
set(VPE_LIB stiVideoPlayback)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiVideoPlayback.cpp
  CstiVideoPlaybackRead.cpp
  PlaybackFlowControl.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiLibraries
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})


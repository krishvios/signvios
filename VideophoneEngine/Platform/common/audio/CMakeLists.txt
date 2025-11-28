# Define the library name
set(VPE_LIB stiPlatform-common-audio)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiAudioBridge.cpp
  BaseAudioInput.cpp
  BaseAudioOutput.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  BaseAudioInput.h
  BaseAudioOutput.h
)

set(${VPE_LIB}_LIBRARIES
  stiOS
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

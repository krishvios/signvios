# Define the library name
set(VPE_LIB stiPlatform)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiPlatform.h
  IstiStatusLED.h
  IstiLightRing.h
  IstiHelios.h
  IstiUserInput.h
  IstiVideoInput.h
  VideoFrame.h
  IstiAudioInput.h
  VideoControl.h
  AudioControl.h
  IstiVideoOutput.h
  IstiVideoPlaybackFrame.h
  IstiAudioOutput.h
  IstiNetwork.h
  IstiMessageViewer.h
  IstiLogging.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiPlatform.h
  IstiStatusLED.h
  IstiLightRing.h
  IstiHelios.h
  IstiUserInput.h
  IstiVideoInput.h
  IstiAudioInput.h
  VideoControl.h
  AudioControl.h
  IstiVideoOutput.h
  IstiAudioOutput.h
  IstiAudibleRinger.h
  IstiMessageViewer.h
  IstiLogging.h
)

set(${VPE_LIB}_LIBRARIES
  stiPlatform-ntouch-vp
  stiPlatform-common
)

set(${VPE_LIB}_SUBDIRECTORIES
  ntouch-vp
  common
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-x86)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiBSPInterface.cpp
  CstiStatusLED.cpp
  CstiLightRing.cpp
  CstiAudioHandler.cpp
  CstiVideoOutput.cpp
  CstiSLICRinger.cpp
  CstiVideoInput.cpp
  CstiMonitorTask.cpp
  CstiAdvancedStatus.cpp
  CstiGStreamerVideoGraph.cpp
  CaptureBin.cpp
  EncodeBin.cpp
  EncodePipeline.cpp
  FrameCaptureBin.cpp
  CropScaleBin.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IAudioInputVP2.h
  ../IVideoInputVP2.h
  ../IVideoOutputVP.h
)

set(${VPE_LIB}_LIBRARIES
  stiPlatform-ntouch-vp-x86-network
)

set(${VPE_LIB}_SUBDIRECTORIES
  network
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ..
  ../common
  ../common/bluetooth
  ../common/dbus 
  ../common/GStreamer
  ../common/MediaPipelines
  ../../common/platform
  ../../common/video
)

set(${VPE_LIB}_LIBRARIES
  stiPlatform-ntouch-vp-common-bluetooth
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

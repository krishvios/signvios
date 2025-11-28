# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-common-GStreamer)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  GStreamerBin.cpp
  GStreamerPad.cpp
  GStreamerElement.cpp
  GStreamerProperty.cpp
  GStreamerPad.cpp
  GStreamerBin.cpp
  GStreamerEvent.cpp
  GStreamerAppSink.h
)

set(${VPE_LIB}_LIBRARIES
  ${Poco_LIBRARIES}
  ${PKG_CFG_GSTREAMER_LIBRARIES}
  ${PKG_CFG_GSTREAMER_APP_LIBRARIES}
  ${PKG_CFG_GSTREAMER_VIDEO_LIBRARIES}
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

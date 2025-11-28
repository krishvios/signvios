# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-common-MediaPipelines)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  AudioRecordPipeline.cpp
  AudioPlaybackPipeline.cpp
  TestAudioInputPipeline.cpp
  AudioFileSourcePipeline.cpp
  AudioPipeline.cpp
  MediaPipeline.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiCommon
  ${PKG_CFG_GSTREAMER_LIBRARIES}
  ${PKG_CFG_GSTREAMER_APP_LIBRARIES}
  ${PKG_CFG_GLIB_LIBRARIES}
  ${PKG_CFG_GTHREAD_LIBRARIES}
  ${PKG_CFG_GIO_LIBRARIES}
  ${PKG_CFG_GIO_UNIX_LIBRARIES}
  ${PKG_CFG_DBUS_GLIB_LIBRARIES}
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ..
  ../GStreamer
  ../bluetooth 
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

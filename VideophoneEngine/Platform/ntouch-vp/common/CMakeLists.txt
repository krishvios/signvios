# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-common)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiBSPInterfaceBase.cpp
  CstiAudioOutput.cpp
  CstiAudibleRinger.cpp
  CstiVideoInputBase.cpp
  CstiVideoOutputBase.cpp
  CstiGStreamerVideoGraphBase.cpp
  CstiCameraControl.cpp
  CstiUserInputBase.cpp
  CstiBufferElement.cpp
  CstiLogging.cpp
  GstreamerBufferElement.cpp
  CstiAudioInputBase.cpp
  H263Tools.cpp
  CstiFileIO.cpp
  FrameCaptureBinBase.cpp
  EncodePipelineBase.cpp
  FanControlBase.cpp
  CaptureBinBase.cpp
  SocketListenerBase.cpp
  CstiVideoOutputBase2.cpp
  CstiGStreamerVideoGraphBase2.cpp
  EncodeBinBase.cpp
  CaptureBinBase2.cpp
  CFilePlayGS.cpp
  DisplaySinkBin.cpp
  DecodePipeline.cpp
  AudioTest.cpp
  JsonTestResponse.cpp
  RecordBinBase.cpp
  SelfViewBinBase.cpp
  EncodePipelineBase2.cpp
  PlaybackPipelineBase.cpp
  CropScaleBinBase.cpp
  CstiMatterStubs.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IPlatformVP.h
  ILightRingVP.h
  ISLICRinger.h
  IFanControl.h
  IUpdate.h
  PropertyRange.h
)

set(${VPE_LIB}_LIBRARIES
  stiPlatform-ntouch-vp-common-bluetooth
  stiPlatform-ntouch-vp-common-dbus
  stiPlatform-ntouch-vp-common-GStreamer
  stiPlatform-ntouch-vp-common-MediaPipelines
  stiFileUpload
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
  ${Poco_LIBRARIES}
  ${PKG_CFG_GSTREAMER_LIBRARIES}
  ${PKG_CFG_GSTREAMER_APP_LIBRARIES}
  ${PKG_CFG_GSTREAMER_VIDEO_LIBRARIES}
)

set(${VPE_LIB}_SUBDIRECTORIES
  bluetooth
  dbus
  GStreamer
  MediaPipelines
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ..
  ../../common/network
  ../../common/platform
  ../../common/video
  bluetooth
  dbus
  GStreamer
  MediaPipelines
  update
)

if("${LUMINA_DEVICE}" STREQUAL "DEV_X86")
  list(APPEND ${VPE_LIB}_LIBRARIES
    stiPlatform-ntouch-vp-x86
    stiPlatform-ntouch-vp-x86-network
    stiPlatform-ntouch-vp-common-update
  )
  list(APPEND ${VPE_LIB}_SUBDIRECTORIES
    update
  )
  list(APPEND ${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
    ../x86
    ../x86/network
  )
else()
  list(APPEND ${VPE_LIB}_SOURCES
    CstiVideoInputBase2.cpp
    MonitorTaskBase2.cpp
    CstiAdvancedStatusBase.cpp
    CstiCECControlBase.cpp
    CstiSLICRingerBase.cpp
    CstiStatusLEDBase.cpp
    CstiLightRingBase.cpp
    CLed.cpp
  )
  list(APPEND ${VPE_LIB}_LIBRARIES
    stiPlatform-ntouch-vp-common-network
  )
  list(APPEND ${VPE_LIB}_SUBDIRECTORIES
    network
  )
endif()

#if("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP2")
#  list(APPEND ${VPE_LIB}_LIBRARIES
#    stiPlatform-ntouch-vp-vp2
#    stiPlatform-ntouch-vp-common-update
#  )
#  list(APPEND ${VPE_LIB}_SUBDIRECTORIES
#    update
#  )
#endif()

if("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP3")
  list(APPEND ${VPE_LIB}_LIBRARIES
    stiPlatform-ntouch-vp-vp3
    ${Libudev_LIBRARIES}
  )
  list(APPEND ${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
    ../vp3
  )
endif()

if("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP4")
  list(APPEND ${VPE_LIB}_LIBRARIES
    stiPlatform-ntouch-vp-vp4
    ${Libudev_LIBRARIES}
  )
  list(APPEND ${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
    ../vp4
  )
endif()

set(${VPE_LIB}_COMPILE_DEFINITIONS
  LINUX
  CONFIG_STA_SUPPORT
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

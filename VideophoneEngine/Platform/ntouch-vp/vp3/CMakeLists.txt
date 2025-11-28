# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-vp3)

pkg_check_modules(PKG_CFG_LIBCAMHAL REQUIRED libcamhal)

if(PKG_CFG_LIBCAMHAL_FOUND)
  message(STATUS "libcamhal found:")
  message(STATUS "  Version: ${PKG_CFG_LIBCAMHAL_VERSION}")
  message(STATUS "  Include directories: ${PKG_CFG_LIBCAMHAL_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${PKG_CFG_LIBCAMHAL_LIBRARIES}")
  message(STATUS "  Definitions: ${PKG_CFG_LIBCAMHAL_DEFINITIONS}")
  message(STATUS "  Compile options: ${PKG_CFG_LIBCAMHAL_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "libcamhal not found")
endif()

find_path(LE9653_API_INCLUDE_DIR api_lib/includes/vp_api.h
          HINTS /usr/include/le9653-api)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiBSPInterface.cpp
  CstiStatusLED.cpp
  CstiLightRing.cpp
  CstiAudioHandler.cpp
  CstiSLICRinger.cpp
  CstiMonitorTask.cpp
  CstiAdvancedStatus.cpp
  CstiVideoOutput.cpp
  CstiVideoInput.cpp
  CstiGStreamerVideoGraph.cpp
  CstiCECControl.cpp
  CstiUserInput.cpp
  CaptureBin.cpp
  EncodeBin.cpp
  EncodePipeline.cpp
  ToFSensor.cpp
  AudioRecordPipelineVP3.cpp
  AudioPlaybackPipelineVP3.cpp
  FrameCaptureBin.cpp
  BluetoothAudio.cpp
  FanControl.cpp
  CstiNetwork.cpp
  CropScaleBin.cpp
)

set(${VPE_LIB}_SUBDIRECTORIES
  update
)

set(${VPE_LIB}_LIBRARIES
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
  ${PKG_CFG_LIBCAMHAL_LIBRARIES}
  stiPlatform-ntouch-vp-vp3-update
  stiPlatform-ntouch-vp-common-bluetooth
  HawkeyeLed
  le9653-api
  stmvl53l1
  gsticamerainterface
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ..
  ../../common/video
  ../../common/platform
  ../common
  ../common/bluetooth
  ../common/GStreamer
  ../common/MediaPipelines
)

set(${VPE_LIB}_INCLUDE_DIRECTORIES
  ${PKG_CFG_LIBCAMHAL_INCLUDE_DIRS}
  ${LE9653_API_INCLUDE_DIR}/api_lib/includes
  ${LE9653_API_INCLUDE_DIR}/api_lib/vp886_api
  ${LE9653_API_INCLUDE_DIR}/arch/x86_spi
  ${LE9653_API_INCLUDE_DIR}/lt_api/includes
  ${LE9653_API_INCLUDE_DIR}/lt_api/vp886_lt
)

set(${VPE_LIB}_COMPILE_OPTIONS
  ${PKG_CFG_LIBCAMHAL_CFLAGS}
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IAudioInputVP2.h
  ../IVideoInputVP2.h
  ../IVideoOutputVP.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-vp4)

find_library(MATTER_BLE NAMES libBleLayer.a PATHS /usr/lib/matter)
find_library(MATTER_CHIP_CORE NAMES libChipCore.a PATHS /usr/lib/matter)
find_library(MATTER_DEVICE NAMES libDeviceLayer.a PATHS /usr/lib/matter)
find_library(MATTER_INET NAMES libInetLayer.a PATHS /usr/lib/matter)
find_library(MATTER_SETUP_PAYLOAD NAMES libSetupPayload.a PATHS /usr/lib/matter)
find_library(MATTER_SUPPORT NAMES libSupportLayer.a PATHS /usr/lib/matter)
find_library(MATTER_LINUX NAMES Linux.a PATHS /usr/lib/matter)
find_library(MATTER_LOGGING_STDIO NAMES liblogging-stdio.a PATHS /usr/lib/matter)
find_library(MATTER_CHIP_UTILS NAMES chip-tool-utils.a PATHS /usr/lib/matter)
find_library(MATTER_CREDENTIALS NAMES libCredentials.a PATHS /usr/lib/matter)
find_library(MATTER_DEFAULT_DEVICE_ATTESTATION NAMES libDefaultAttestationVerifier.a PATHS /usr/lib/matter)

find_path(MATTER_INCLUDE_DIR SystemProjectConfig.h
          HINTS /usr/include/matter)

find_path(MATTER_NLASSERT_INCLUDE_DIR nlassert.h
          HINTS /usr/include/matter/third_party/nlassert/repo/include)

find_path(MATTER_TRACING_INCLUDE_DIR TracingCommandLineArgument.h
          HINTS /usr/include/matter/tracing)

find_path(MATTER_NLIO_INCLUDE_DIR nlbyteorder.hpp
          HINTS /usr/include/matter/third_party/nlio/repo/include)

find_path(MATTER_PERFETTO_INCLUDE_DIR perfetto.h
          HINTS /usr/include/matter/third_party/perfetto/repo/sdk)

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
  AudioRecordPipelineVP4.cpp
  AudioPlaybackPipelineVP4.cpp
  FrameCaptureBin.cpp
  BluetoothAudio.cpp
  CstiNetwork.cpp
  CropScaleBin.cpp
#  CstiMatter.cpp
)

set(${VPE_LIB}_LIBRARIES
# We have circular linking (i.e. A needs B, B needs C and C needs A). Libs are included twice to solve the issue.\
#  -Wl,--whole-archive
#  ${MATTER_BLE}
#  ${MATTER_CHIP_CORE}
#  ${MATTER_DEVICE}
#  ${MATTER_INET}
#  ${MATTER_SETUP_PAYLOAD}
#  ${MATTER_SUPPORT}
#  ${MATTER_LINUX}
#  ${MATTER_LOGGING_STDIO}
#  ${MATTER_CHIP_UTILS}
#  ${MATTER_CREDENTIALS}
#  ${MATTER_DEFAULT_DEVICE_ATTESTATION}
#  ${MATTER_BLE}
#  ${MATTER_CHIP_CORE}
#  ${MATTER_DEVICE}
#  ${MATTER_INET}
#  ${MATTER_SETUP_PAYLOAD}
#  ${MATTER_SUPPORT}
#  ${MATTER_LINUX}
#  ${MATTER_LOGGING_STDIO}
#  ${MATTER_CHIP_UTILS}
#  ${MATTER_CREDENTIALS}
#  ${MATTER_DEFAULT_DEVICE_ATTESTATION}
#  -Wl,--no-whole-archive
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
  stiPlatform-ntouch-vp-vp4-update
  stiPlatform-ntouch-vp-common-bluetooth
  bluealsa
  SorensonLed
  SorensonVvcam
  curl
  jsoncpp
)

set(${VPE_LIB}_SUBDIRECTORIES
  update
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ..
  ../../common/video
  ../../common/platform
  ../common
  ../common/bluetooth
  ../common/GStreamer
  ../common/MediaPipelines
  ${MATTER_INCLUDE_DIR}
  ${MATTER_NLASSERT_INCLUDE_DIR}
  ${MATTER_TRACING_INCLUDE_DIR}
  ${MATTER_NLIO_INCLUDE_DIR}
  ${MATTER_PERFETTO_INCLUDE_DIR}
)

set(${VPE_LIB}_COMPILE_DEFINITIONS
  LINUX
  HAL_ALTERA
  CHIP_HAVE_CONFIG_H
  CHIP_SYSTEM_CONFIG_USE_SOCKETS
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

#target_link_libraries(${VPE_LIB}
#  PRIVATE
#  -Wl,--whole-archive ${MATTER_DEVICE} -Wl,--no-whole-archive
#)

# Define the library name
set(VPE_LIB stiPlatform-common)

set(${VPE_LIB}_LIBRARIES
  stiPlatform-common-audio
  stiPlatform-common-bluetooth
  stiPlatform-common-network
  stiPlatform-common-platform
  stiPlatform-common-video
  stiPlatform-common-wireless
  stiOS
  stiFileUpload
)

set(${VPE_LIB}_SUBDIRECTORIES
  audio
  bluetooth
  network
  platform
  video
  wireless
)

if("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP2")
  list(APPEND ${VPE_LIB}_LIBRARIES
    stiPlatform-common-fileplay
  )
  list(APPEND ${VPE_LIB}_SUBDIRECTORIES
    fileplay
  )
endif()

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

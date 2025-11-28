# Define the library name
set(VPE_LIB stiDataTasks)

set(${VPE_LIB}_LIBRARIES
  stiAudioPlayback
  stiAudioRecord
  stiDataCommon
  stiTextPlayback
  stiTextRecord
  stiVideoPlayback
  stiVideoRecord
)

set(${VPE_LIB}_SUBDIRECTORIES
  AudioPlayback
  AudioRecord
  DataCommon
  TextPlayback
  TextRecord
  VideoPlayback
  VideoRecord
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

# Define the library name
set(VPE_LIB stiVideoRecord)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiVideoRecord.cpp
  RecordFlowControl.cpp
  RateHistoryManager.cpp
  Packetization.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiDataCommon
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})


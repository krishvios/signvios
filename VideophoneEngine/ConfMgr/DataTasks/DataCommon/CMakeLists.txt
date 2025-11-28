# Define the library name
set(VPE_LIB stiDataCommon)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiRtpHeaderLength.cpp
  CstiDataTaskCommonP.cpp
  CstiSyncManager.cpp
  RTCPFlowControl.cpp
  RtpPayloadAddon.cpp
  utf8EncodeDecode.cpp
  CstiRtpSession.cpp
  CstiAesEncryptionContext.cpp
  SDESKey.cpp
  DtlsContext.cpp
  RtxSender.cpp
  RtpSender.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiLibraries
)

set(${VPE_LIB}_COMPILE_OPTIONS
  -Wno-deprecated-declarations
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

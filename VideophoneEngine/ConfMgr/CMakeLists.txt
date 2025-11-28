# Define the library name
set(VPE_LIB stiConferenceManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiCall.cpp
  CstiCall.h
  CstiCallInfo.cpp
  CstiCallStorage.cpp
  CstiConferenceManager.cpp
  CstiProtocolCall.cpp
  CstiProtocolManager.cpp
  stiSystemInfo.cpp
  CstiRoutingAddress.cpp
  stiSipTools.cpp
  CstiSipRegistration.cpp
  CstiSipManager.cpp
  CstiSipCall.cpp
  CstiSipCallLeg.cpp
  CstiSipResolver.cpp
  stiSipStatistics.cpp
  DhvClient.cpp
  CstiTunnelManager.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiDataTasks
  stiNat
  stiSession
  stiSip
  stiLibraries
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
  ${Poco_LIBRARIES}
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  ICallInfo.h
  IstiCall.h
  IVRCLCall.h
  IstiTunnelManager.h
  stiConfDefs.h
  CstiServerInfo.h
  CORP_CA.cer.pem
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  ICallInfo.h
  IstiCall.h
)

set(${VPE_LIB}_SUBDIRECTORIES
  DataTasks
  Nat
  Session
  Sip
)


set(${VPE_LIB}_COMPILE_OPTIONS
  -Wno-undefined-bool-conversion
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
